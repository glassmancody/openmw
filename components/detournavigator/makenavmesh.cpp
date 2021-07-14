#include "makenavmesh.hpp"
#include "debug.hpp"
#include "exceptions.hpp"
#include "recastmesh.hpp"
#include "settings.hpp"
#include "settingsutils.hpp"
#include "sharednavmesh.hpp"
#include "flags.hpp"
#include "navmeshtilescache.hpp"
#include "preparednavmeshdata.hpp"
#include "navmeshdata.hpp"

#include <components/misc/convert.hpp>

#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>
#include <Recast.h>
#include <RecastAlloc.h>

#include <components/debug/debuglog.hpp>

#include <algorithm>
#include <iomanip>
#include <limits>
#include <array>

namespace
{
    using namespace DetourNavigator;

    struct WaterBounds
    {
        osg::Vec3f mMin;
        osg::Vec3f mMax;
    };

    WaterBounds getWaterBounds(const Cell& water, const Settings& settings,
        const osg::Vec3f& agentHalfExtents)
    {
        if (water.mSize == std::numeric_limits<int>::max())
        {
            const auto transform = getSwimLevelTransform(settings, water.mShift, agentHalfExtents.z());
            const auto min = toNavMeshCoordinates(settings, Misc::Convert::makeOsgVec3f(transform(btVector3(-1, -1, 0))));
            const auto max = toNavMeshCoordinates(settings, Misc::Convert::makeOsgVec3f(transform(btVector3(1, 1, 0))));
            return WaterBounds {
                osg::Vec3f(-std::numeric_limits<float>::max(), min.y(), -std::numeric_limits<float>::max()),
                osg::Vec3f(std::numeric_limits<float>::max(), max.y(), std::numeric_limits<float>::max())
            };
        }
        else
        {
            const auto transform = getSwimLevelTransform(settings, water.mShift, agentHalfExtents.z());
            const auto halfCellSize = water.mSize / 2.0f;
            return WaterBounds {
                toNavMeshCoordinates(settings, Misc::Convert::makeOsgVec3f(transform(btVector3(-halfCellSize, -halfCellSize, 0)))),
                toNavMeshCoordinates(settings, Misc::Convert::makeOsgVec3f(transform(btVector3(halfCellSize, halfCellSize, 0))))
            };
        }
    }

    std::vector<float> getOffMeshVerts(const std::vector<OffMeshConnection>& connections)
    {
        std::vector<float> result;

        result.reserve(connections.size() * 6);

        const auto add = [&] (const osg::Vec3f& v)
        {
            result.push_back(v.x());
            result.push_back(v.y());
            result.push_back(v.z());
        };

        for (const auto& v : connections)
        {
            add(v.mStart);
            add(v.mEnd);
        }

        return result;
    }

    Flag getFlag(AreaType areaType)
    {
        switch (areaType)
        {
            case AreaType_null:
                return Flag_none;
            case AreaType_ground:
                return Flag_walk;
            case AreaType_water:
                return Flag_swim;
            case AreaType_door:
                return Flag_openDoor;
            case AreaType_pathgrid:
                return Flag_usePathgrid;
        }
        return Flag_none;
    }

    std::vector<unsigned char> getOffMeshConAreas(const std::vector<OffMeshConnection>& connections)
    {
        std::vector<unsigned char> result;
        result.reserve(connections.size());
        std::transform(connections.begin(), connections.end(), std::back_inserter(result),
                       [] (const OffMeshConnection& v) { return v.mAreaType; });
        return result;
    }

    std::vector<unsigned short> getOffMeshFlags(const std::vector<OffMeshConnection>& connections)
    {
        std::vector<unsigned short> result;
        result.reserve(connections.size());
        std::transform(connections.begin(), connections.end(), std::back_inserter(result),
                       [] (const OffMeshConnection& v) { return getFlag(v.mAreaType); });
        return result;
    }

    rcConfig makeConfig(const osg::Vec3f& agentHalfExtents, const osg::Vec3f& boundsMin, const osg::Vec3f& boundsMax,
        const Settings& settings)
    {
        rcConfig config;

        config.cs = settings.mCellSize;
        config.ch = settings.mCellHeight;
        config.walkableSlopeAngle = settings.mMaxSlope;
        config.walkableHeight = static_cast<int>(std::ceil(getHeight(settings, agentHalfExtents) / config.ch));
        config.walkableClimb = static_cast<int>(std::floor(getMaxClimb(settings) / config.ch));
        config.walkableRadius = static_cast<int>(std::ceil(getRadius(settings, agentHalfExtents) / config.cs));
        config.maxEdgeLen = static_cast<int>(std::round(settings.mMaxEdgeLen / config.cs));
        config.maxSimplificationError = settings.mMaxSimplificationError;
        config.minRegionArea = settings.mRegionMinSize * settings.mRegionMinSize;
        config.mergeRegionArea = settings.mRegionMergeSize * settings.mRegionMergeSize;
        config.maxVertsPerPoly = settings.mMaxVertsPerPoly;
        config.detailSampleDist = settings.mDetailSampleDist < 0.9f ? 0 : config.cs * settings.mDetailSampleDist;
        config.detailSampleMaxError = config.ch * settings.mDetailSampleMaxError;
        config.borderSize = settings.mBorderSize;
        config.width = settings.mTileSize + config.borderSize * 2;
        config.height = settings.mTileSize + config.borderSize * 2;
        rcVcopy(config.bmin, boundsMin.ptr());
        rcVcopy(config.bmax, boundsMax.ptr());
        config.bmin[0] -= getBorderSize(settings);
        config.bmin[2] -= getBorderSize(settings);
        config.bmax[0] += getBorderSize(settings);
        config.bmax[2] += getBorderSize(settings);
        config.tileSize = settings.mTileSize;

        return config;
    }

    void createHeightfield(rcContext& context, rcHeightfield& solid, int width, int height, const float* bmin,
        const float* bmax, const float cs, const float ch)
    {
        const auto result = rcCreateHeightfield(&context, solid, width, height, bmin, bmax, cs, ch);

        if (!result)
            throw NavigatorException("Failed to create heightfield for navmesh");
    }

    bool rasterizeTriangles(rcContext& context, const Mesh& mesh, const Settings& settings, const rcConfig& config,
        rcHeightfield& solid)
    {
        std::vector<unsigned char> areas(mesh.getAreaTypes().begin(), mesh.getAreaTypes().end());
        std::vector<float> vertices = mesh.getVertices();

        for (std::size_t i = 0; i < vertices.size(); i += 3)
        {
            for (std::size_t j = 0; j < 3; ++j)
                vertices[i + j] = toNavMeshCoordinates(settings, vertices[i + j]);
            std::swap(vertices[i + 1], vertices[i + 2]);
        }

        rcClearUnwalkableTriangles(
            &context,
            config.walkableSlopeAngle,
            vertices.data(),
            static_cast<int>(mesh.getVerticesCount()),
            mesh.getIndices().data(),
            static_cast<int>(areas.size()),
            areas.data()
        );

        return rcRasterizeTriangles(
            &context,
            vertices.data(),
            static_cast<int>(mesh.getVerticesCount()),
            mesh.getIndices().data(),
            areas.data(),
            static_cast<int>(areas.size()),
            solid,
            config.walkableClimb
        );
    }

    void rasterizeWaterTriangles(rcContext& context, const osg::Vec3f& agentHalfExtents, const RecastMesh& recastMesh,
        const Settings& settings, const rcConfig& config, rcHeightfield& solid)
    {
        const std::array<unsigned char, 2> areas {{AreaType_water, AreaType_water}};

        for (const auto& water : recastMesh.getWater())
        {
            const auto bounds = getWaterBounds(water, settings, agentHalfExtents);

            const osg::Vec2f tileBoundsMin(
                std::min(config.bmax[0], std::max(config.bmin[0], bounds.mMin.x())),
                std::min(config.bmax[2], std::max(config.bmin[2], bounds.mMin.z()))
            );
            const osg::Vec2f tileBoundsMax(
                std::min(config.bmax[0], std::max(config.bmin[0], bounds.mMax.x())),
                std::min(config.bmax[2], std::max(config.bmin[2], bounds.mMax.z()))
            );

            if (tileBoundsMax == tileBoundsMin)
                continue;

            const std::array<osg::Vec3f, 4> vertices {{
                osg::Vec3f(tileBoundsMin.x(), bounds.mMin.y(), tileBoundsMin.y()),
                osg::Vec3f(tileBoundsMin.x(), bounds.mMin.y(), tileBoundsMax.y()),
                osg::Vec3f(tileBoundsMax.x(), bounds.mMin.y(), tileBoundsMax.y()),
                osg::Vec3f(tileBoundsMax.x(), bounds.mMin.y(), tileBoundsMin.y()),
            }};

            std::array<float, 4 * 3> convertedVertices;
            auto convertedVerticesIt = convertedVertices.begin();

            for (const auto& vertex : vertices)
                convertedVerticesIt = std::copy(vertex.ptr(), vertex.ptr() + 3, convertedVerticesIt);

            const std::array<int, 6> indices {{
                0, 1, 2,
                0, 2, 3,
            }};

            const auto trianglesRasterized = rcRasterizeTriangles(
                &context,
                convertedVertices.data(),
                static_cast<int>(convertedVertices.size() / 3),
                indices.data(),
                areas.data(),
                static_cast<int>(areas.size()),
                solid,
                config.walkableClimb
            );

            if (!trianglesRasterized)
                throw NavigatorException("Failed to create rasterize water triangles for navmesh");
        }
    }

    bool rasterizeTriangles(rcContext& context, const osg::Vec3f& agentHalfExtents, const RecastMesh& recastMesh,
        const rcConfig& config, const Settings& settings, rcHeightfield& solid)
    {
        if (!rasterizeTriangles(context, recastMesh.getMesh(), settings, config, solid))
            return false;

        rasterizeWaterTriangles(context, agentHalfExtents, recastMesh, settings, config, solid);

        return true;
    }

    void buildCompactHeightfield(rcContext& context, const int walkableHeight, const int walkableClimb,
                                 rcHeightfield& solid, rcCompactHeightfield& compact)
    {
        const auto result = rcBuildCompactHeightfield(&context, walkableHeight,
            walkableClimb, solid, compact);

        if (!result)
            throw NavigatorException("Failed to build compact heightfield for navmesh");
    }

    void erodeWalkableArea(rcContext& context, int walkableRadius, rcCompactHeightfield& compact)
    {
        const auto result = rcErodeWalkableArea(&context, walkableRadius, compact);

        if (!result)
            throw NavigatorException("Failed to erode walkable area for navmesh");
    }

    void buildDistanceField(rcContext& context, rcCompactHeightfield& compact)
    {
        const auto result = rcBuildDistanceField(&context, compact);

        if (!result)
            throw NavigatorException("Failed to build distance field for navmesh");
    }

    void buildRegions(rcContext& context, rcCompactHeightfield& compact, const int borderSize,
        const int minRegionArea, const int mergeRegionArea)
    {
        const auto result = rcBuildRegions(&context, compact, borderSize, minRegionArea, mergeRegionArea);

        if (!result)
            throw NavigatorException("Failed to build distance field for navmesh");
    }

    void buildContours(rcContext& context, rcCompactHeightfield& compact, const float maxError, const int maxEdgeLen,
        rcContourSet& contourSet, const int buildFlags = RC_CONTOUR_TESS_WALL_EDGES)
    {
        const auto result = rcBuildContours(&context, compact, maxError, maxEdgeLen, contourSet, buildFlags);

        if (!result)
            throw NavigatorException("Failed to build contours for navmesh");
    }

    void buildPolyMesh(rcContext& context, rcContourSet& contourSet, const int maxVertsPerPoly, rcPolyMesh& polyMesh)
    {
        const auto result = rcBuildPolyMesh(&context, contourSet, maxVertsPerPoly, polyMesh);

        if (!result)
            throw NavigatorException("Failed to build poly mesh for navmesh");
    }

    void buildPolyMeshDetail(rcContext& context, const rcPolyMesh& polyMesh, const rcCompactHeightfield& compact,
        const float sampleDist, const float sampleMaxError, rcPolyMeshDetail& polyMeshDetail)
    {
        const auto result = rcBuildPolyMeshDetail(&context, polyMesh, compact, sampleDist, sampleMaxError,
                                                  polyMeshDetail);

        if (!result)
            throw NavigatorException("Failed to build detail poly mesh for navmesh");
    }

    void setPolyMeshFlags(rcPolyMesh& polyMesh)
    {
        for (int i = 0; i < polyMesh.npolys; ++i)
            polyMesh.flags[i] = getFlag(static_cast<AreaType>(polyMesh.areas[i]));
    }

    bool fillPolyMesh(rcContext& context, const rcConfig& config, rcHeightfield& solid, rcPolyMesh& polyMesh,
        rcPolyMeshDetail& polyMeshDetail)
    {
        rcCompactHeightfield compact;
        buildCompactHeightfield(context, config.walkableHeight, config.walkableClimb, solid, compact);

        erodeWalkableArea(context, config.walkableRadius, compact);
        buildDistanceField(context, compact);
        buildRegions(context, compact, config.borderSize, config.minRegionArea, config.mergeRegionArea);

        rcContourSet contourSet;
        buildContours(context, compact, config.maxSimplificationError, config.maxEdgeLen, contourSet);

        if (contourSet.nconts == 0)
            return false;

        buildPolyMesh(context, contourSet, config.maxVertsPerPoly, polyMesh);

        buildPolyMeshDetail(context, polyMesh, compact, config.detailSampleDist, config.detailSampleMaxError,
                            polyMeshDetail);

        setPolyMeshFlags(polyMesh);

        return true;
    }

    template <class T>
    unsigned long getMinValuableBitsNumber(const T value)
    {
        unsigned long power = 0;
        while (power < sizeof(T) * 8 && (static_cast<T>(1) << power) < value)
            ++power;
        return power;
    }
}

namespace DetourNavigator
{
    std::unique_ptr<PreparedNavMeshData> prepareNavMeshTileData(const RecastMesh& recastMesh,
        const TilePosition& tile, const Bounds& bounds, const osg::Vec3f& agentHalfExtents, const Settings& settings)
    {
        const TileBounds tileBounds = makeTileBounds(settings, tile);
        const osg::Vec3f boundsMin(tileBounds.mMin.x(), bounds.mMin.y() - 1, tileBounds.mMin.y());
        const osg::Vec3f boundsMax(tileBounds.mMax.x(), bounds.mMax.y() + 1, tileBounds.mMax.y());

        rcContext context;
        const auto config = makeConfig(agentHalfExtents, boundsMin, boundsMax, settings);

        rcHeightfield solid;
        createHeightfield(context, solid, config.width, config.height, config.bmin, config.bmax, config.cs, config.ch);

        if (!rasterizeTriangles(context, agentHalfExtents, recastMesh, config, settings, solid))
            return nullptr;

        rcFilterLowHangingWalkableObstacles(&context, config.walkableClimb, solid);
        rcFilterLedgeSpans(&context, config.walkableHeight, config.walkableClimb, solid);
        rcFilterWalkableLowHeightSpans(&context, config.walkableHeight, solid);

        std::unique_ptr<PreparedNavMeshData> result = std::make_unique<PreparedNavMeshData>();

        if (!fillPolyMesh(context, config, solid, result->mPolyMesh, result->mPolyMeshDetail))
            return nullptr;

        result->mCellSize = config.cs;
        result->mCellHeight = config.ch;

        return result;
    }

    NavMeshData makeNavMeshTileData(const PreparedNavMeshData& data,
        const std::vector<OffMeshConnection>& offMeshConnections, const osg::Vec3f& agentHalfExtents,
        const TilePosition& tile, const Settings& settings)
    {
        const auto offMeshConVerts = getOffMeshVerts(offMeshConnections);
        const std::vector<float> offMeshConRad(offMeshConnections.size(), getRadius(settings, agentHalfExtents));
        const std::vector<unsigned char> offMeshConDir(offMeshConnections.size(), 0);
        const std::vector<unsigned char> offMeshConAreas = getOffMeshConAreas(offMeshConnections);
        const std::vector<unsigned short> offMeshConFlags = getOffMeshFlags(offMeshConnections);

        dtNavMeshCreateParams params;
        params.verts = data.mPolyMesh.verts;
        params.vertCount = data.mPolyMesh.nverts;
        params.polys = data.mPolyMesh.polys;
        params.polyAreas = data.mPolyMesh.areas;
        params.polyFlags = data.mPolyMesh.flags;
        params.polyCount = data.mPolyMesh.npolys;
        params.nvp = data.mPolyMesh.nvp;
        params.detailMeshes = data.mPolyMeshDetail.meshes;
        params.detailVerts = data.mPolyMeshDetail.verts;
        params.detailVertsCount = data.mPolyMeshDetail.nverts;
        params.detailTris = data.mPolyMeshDetail.tris;
        params.detailTriCount = data.mPolyMeshDetail.ntris;
        params.offMeshConVerts = offMeshConVerts.data();
        params.offMeshConRad = offMeshConRad.data();
        params.offMeshConDir = offMeshConDir.data();
        params.offMeshConAreas = offMeshConAreas.data();
        params.offMeshConFlags = offMeshConFlags.data();
        params.offMeshConUserID = nullptr;
        params.offMeshConCount = static_cast<int>(offMeshConnections.size());
        params.walkableHeight = getHeight(settings, agentHalfExtents);
        params.walkableRadius = getRadius(settings, agentHalfExtents);
        params.walkableClimb = getMaxClimb(settings);
        rcVcopy(params.bmin, data.mPolyMesh.bmin);
        rcVcopy(params.bmax, data.mPolyMesh.bmax);
        params.cs = data.mCellSize;
        params.ch = data.mCellHeight;
        params.buildBvTree = true;
        params.userId = data.mUserId;
        params.tileX = tile.x();
        params.tileY = tile.y();
        params.tileLayer = 0;

        unsigned char* navMeshData;
        int navMeshDataSize;
        const auto navMeshDataCreated = dtCreateNavMeshData(&params, &navMeshData, &navMeshDataSize);

        if (!navMeshDataCreated)
            throw NavigatorException("Failed to create navmesh tile data");

        return NavMeshData(navMeshData, navMeshDataSize);
    }

    NavMeshPtr makeEmptyNavMesh(const Settings& settings)
    {
        // Max tiles and max polys affect how the tile IDs are caculated.
        // There are 22 bits available for identifying a tile and a polygon.
        const int polysAndTilesBits = 22;
        const auto polysBits = getMinValuableBitsNumber(settings.mMaxPolys);

        if (polysBits >= polysAndTilesBits)
            throw InvalidArgument("Too many polygons per tile");

        const auto tilesBits = polysAndTilesBits - polysBits;

        dtNavMeshParams params;
        std::fill_n(params.orig, 3, 0.0f);
        params.tileWidth = settings.mTileSize * settings.mCellSize;
        params.tileHeight = settings.mTileSize * settings.mCellSize;
        params.maxTiles = 1 << tilesBits;
        params.maxPolys = 1 << polysBits;

        NavMeshPtr navMesh(dtAllocNavMesh(), &dtFreeNavMesh);
        const auto status = navMesh->init(&params);

        if (!dtStatusSucceed(status))
            throw NavigatorException("Failed to init navmesh");

        return navMesh;
    }

    UpdateNavMeshStatus updateNavMesh(const osg::Vec3f& agentHalfExtents, const RecastMesh* recastMesh,
        const TilePosition& changedTile, const TilePosition& playerTile,
        const std::vector<OffMeshConnection>& offMeshConnections, const Settings& settings,
        const SharedNavMeshCacheItem& navMeshCacheItem, NavMeshTilesCache& navMeshTilesCache)
    {
        Log(Debug::Debug) << std::fixed << std::setprecision(2) <<
            "Update NavMesh with multiple tiles:" <<
            " agentHeight=" << getHeight(settings, agentHalfExtents) <<
            " agentMaxClimb=" << getMaxClimb(settings) <<
            " agentRadius=" << getRadius(settings, agentHalfExtents) <<
            " changedTile=(" << changedTile << ")" <<
            " playerTile=(" << playerTile << ")" <<
            " changedTileDistance=" << getDistance(changedTile, playerTile);

        const auto params = *navMeshCacheItem->lockConst()->getImpl().getParams();

        if (!recastMesh)
        {
            Log(Debug::Debug) << "Ignore add tile: recastMesh is null";
            return navMeshCacheItem->lock()->removeTile(changedTile);
        }

        auto recastMeshBounds = recastMesh->getBounds();
        recastMeshBounds.mMin = toNavMeshCoordinates(settings, recastMeshBounds.mMin);
        recastMeshBounds.mMax = toNavMeshCoordinates(settings, recastMeshBounds.mMax);

        for (const auto& water : recastMesh->getWater())
        {
            const auto waterBounds = getWaterBounds(water, settings, agentHalfExtents);
            recastMeshBounds.mMin.y() = std::min(recastMeshBounds.mMin.y(), waterBounds.mMin.y());
            recastMeshBounds.mMax.y() = std::max(recastMeshBounds.mMax.y(), waterBounds.mMax.y());
        }

        if (isEmpty(recastMeshBounds))
        {
            Log(Debug::Debug) << "Ignore add tile: recastMesh is empty";
            return navMeshCacheItem->lock()->removeTile(changedTile);
        }

        if (!shouldAddTile(changedTile, playerTile, std::min(settings.mMaxTilesNumber, params.maxTiles)))
        {
            Log(Debug::Debug) << "Ignore add tile: too far from player";
            return navMeshCacheItem->lock()->removeTile(changedTile);
        }

        auto cachedNavMeshData = navMeshTilesCache.get(agentHalfExtents, changedTile, *recastMesh);
        bool cached = static_cast<bool>(cachedNavMeshData);

        if (!cachedNavMeshData)
        {
            auto prepared = prepareNavMeshTileData(*recastMesh, changedTile, recastMeshBounds,
                                                   agentHalfExtents, settings);

            if (prepared == nullptr)
            {
                Log(Debug::Debug) << "Ignore add tile: NavMeshData is null";
                return navMeshCacheItem->lock()->removeTile(changedTile);
            }

            cachedNavMeshData = navMeshTilesCache.set(agentHalfExtents, changedTile, *recastMesh, std::move(prepared));

            if (!cachedNavMeshData)
            {
                Log(Debug::Debug) << "Navigator cache overflow";
                return navMeshCacheItem->lock()->updateTile(changedTile, NavMeshTilesCache::Value(),
                    makeNavMeshTileData(*prepared, offMeshConnections, agentHalfExtents, changedTile, settings));
            }
        }

        const auto updateStatus = navMeshCacheItem->lock()->updateTile(changedTile, std::move(cachedNavMeshData),
            makeNavMeshTileData(cachedNavMeshData.get(), offMeshConnections, agentHalfExtents, changedTile, settings));

        return UpdateNavMeshStatusBuilder(updateStatus).cached(cached).getResult();
    }
}
