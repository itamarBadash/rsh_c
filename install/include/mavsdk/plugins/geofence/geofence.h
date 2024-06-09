// WARNING: THIS FILE IS AUTOGENERATED! As such, it should not be edited.
// Edits need to be made to the proto files
// (see https://github.com/mavlink/MAVSDK-Proto/blob/main/protos/geofence/geofence.proto)

#pragma once

#include <array>
#include <cmath>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "plugin_base.h"

#include "handle.h"

namespace mavsdk {

class System;
class GeofenceImpl;

/**
 * @brief Enable setting a geofence.
 */
class Geofence : public PluginBase {
public:
    /**
     * @brief Constructor. Creates the plugin for a specific System.
     *
     * The plugin is typically created as shown below:
     *
     *     ```cpp
     *     auto geofence = Geofence(system);
     *     ```
     *
     * @param system The specific system associated with this plugin.
     */
    explicit Geofence(System& system); // deprecated

    /**
     * @brief Constructor. Creates the plugin for a specific System.
     *
     * The plugin is typically created as shown below:
     *
     *     ```cpp
     *     auto geofence = Geofence(system);
     *     ```
     *
     * @param system The specific system associated with this plugin.
     */
    explicit Geofence(std::shared_ptr<System> system); // new

    /**
     * @brief Destructor (internal use only).
     */
    ~Geofence() override;

    /**
     * @brief Geofence types.
     */
    enum class FenceType {
        Inclusion, /**< @brief Type representing an inclusion fence. */
        Exclusion, /**< @brief Type representing an exclusion fence. */
    };

    /**
     * @brief Stream operator to print information about a `Geofence::FenceType`.
     *
     * @return A reference to the stream.
     */
    friend std::ostream& operator<<(std::ostream& str, Geofence::FenceType const& fence_type);

    /**
     * @brief Point type.
     */
    struct Point {
        double latitude_deg{}; /**< @brief Latitude in degrees (range: -90 to +90) */
        double longitude_deg{}; /**< @brief Longitude in degrees (range: -180 to +180) */
    };

    /**
     * @brief Equal operator to compare two `Geofence::Point` objects.
     *
     * @return `true` if items are equal.
     */
    friend bool operator==(const Geofence::Point& lhs, const Geofence::Point& rhs);

    /**
     * @brief Stream operator to print information about a `Geofence::Point`.
     *
     * @return A reference to the stream.
     */
    friend std::ostream& operator<<(std::ostream& str, Geofence::Point const& point);

    /**
     * @brief Polygon type.
     */
    struct Polygon {
        std::vector<Point> points{}; /**< @brief Points defining the polygon */
        FenceType fence_type{}; /**< @brief Fence type */
    };

    /**
     * @brief Equal operator to compare two `Geofence::Polygon` objects.
     *
     * @return `true` if items are equal.
     */
    friend bool operator==(const Geofence::Polygon& lhs, const Geofence::Polygon& rhs);

    /**
     * @brief Stream operator to print information about a `Geofence::Polygon`.
     *
     * @return A reference to the stream.
     */
    friend std::ostream& operator<<(std::ostream& str, Geofence::Polygon const& polygon);

    /**
     * @brief Circular type.
     */
    struct Circle {
        Point point{}; /**< @brief Point defining the center */
        float radius{float(NAN)}; /**< @brief Radius of the circular fence */
        FenceType fence_type{}; /**< @brief Fence type */
    };

    /**
     * @brief Equal operator to compare two `Geofence::Circle` objects.
     *
     * @return `true` if items are equal.
     */
    friend bool operator==(const Geofence::Circle& lhs, const Geofence::Circle& rhs);

    /**
     * @brief Stream operator to print information about a `Geofence::Circle`.
     *
     * @return A reference to the stream.
     */
    friend std::ostream& operator<<(std::ostream& str, Geofence::Circle const& circle);

    /**
     * @brief Geofence data type.
     */
    struct GeofenceData {
        std::vector<Polygon> polygons{}; /**< @brief Polygon(s) representing the geofence(s) */
        std::vector<Circle> circles{}; /**< @brief Circle(s) representing the geofence(s) */
    };

    /**
     * @brief Equal operator to compare two `Geofence::GeofenceData` objects.
     *
     * @return `true` if items are equal.
     */
    friend bool operator==(const Geofence::GeofenceData& lhs, const Geofence::GeofenceData& rhs);

    /**
     * @brief Stream operator to print information about a `Geofence::GeofenceData`.
     *
     * @return A reference to the stream.
     */
    friend std::ostream& operator<<(std::ostream& str, Geofence::GeofenceData const& geofence_data);

    /**
     * @brief Possible results returned for geofence requests.
     */
    enum class Result {
        Unknown, /**< @brief Unknown result. */
        Success, /**< @brief Request succeeded. */
        Error, /**< @brief Error. */
        TooManyGeofenceItems, /**< @brief Too many objects in the geofence. */
        Busy, /**< @brief Vehicle is busy. */
        Timeout, /**< @brief Request timed out. */
        InvalidArgument, /**< @brief Invalid argument. */
        NoSystem, /**< @brief No system connected. */
    };

    /**
     * @brief Stream operator to print information about a `Geofence::Result`.
     *
     * @return A reference to the stream.
     */
    friend std::ostream& operator<<(std::ostream& str, Geofence::Result const& result);

    /**
     * @brief Callback type for asynchronous Geofence calls.
     */
    using ResultCallback = std::function<void(Result)>;

    /**
     * @brief Upload geofences.
     *
     * Polygon and Circular geofences are uploaded to a drone. Once uploaded, the geofence will
     * remain on the drone even if a connection is lost.
     *
     * This function is non-blocking. See 'upload_geofence' for the blocking counterpart.
     */
    void upload_geofence_async(GeofenceData geofence_data, const ResultCallback callback);

    /**
     * @brief Upload geofences.
     *
     * Polygon and Circular geofences are uploaded to a drone. Once uploaded, the geofence will
     * remain on the drone even if a connection is lost.
     *
     * This function is blocking. See 'upload_geofence_async' for the non-blocking counterpart.
     *
     * @return Result of request.
     */
    Result upload_geofence(GeofenceData geofence_data) const;

    /**
     * @brief Clear all geofences saved on the vehicle.
     *
     * This function is non-blocking. See 'clear_geofence' for the blocking counterpart.
     */
    void clear_geofence_async(const ResultCallback callback);

    /**
     * @brief Clear all geofences saved on the vehicle.
     *
     * This function is blocking. See 'clear_geofence_async' for the non-blocking counterpart.
     *
     * @return Result of request.
     */
    Result clear_geofence() const;

    /**
     * @brief Copy constructor.
     */
    Geofence(const Geofence& other);

    /**
     * @brief Equality operator (object is not copyable).
     */
    const Geofence& operator=(const Geofence&) = delete;

private:
    /** @private Underlying implementation, set at instantiation */
    std::unique_ptr<GeofenceImpl> _impl;
};

} // namespace mavsdk