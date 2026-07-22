#pragma once

#include "pch.hpp"

#include <QHash>
#include <QPointF>
#include <QQuickItem>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>
#include <QWindow>

#include <memory>

class UiOsgInitMapWindow;

class UiOsgInitMap : public QQuickItem
{
	Q_OBJECT

	Q_PROPERTY(QWindow* mapWindow READ mapWindow CONSTANT)
	Q_PROPERTY(QVariantList platformRows READ platformRows WRITE setPlatformRows NOTIFY platformRowsChanged)
	Q_PROPERTY(QString selectedPlatformId READ selectedPlatformId WRITE setSelectedPlatformId NOTIFY selectedPlatformIdChanged)
	Q_PROPERTY(bool showPlatformId READ showPlatformId WRITE setShowPlatformId NOTIFY showPlatformIdChanged)
	Q_PROPERTY(bool showCommandLink READ showCommandLink WRITE setShowCommandLink NOTIFY showCommandLinkChanged)
	Q_PROPERTY(bool showSensorRange READ showSensorRange WRITE setShowSensorRange NOTIFY showSensorRangeChanged)
	Q_PROPERTY(bool showWeaponRange READ showWeaponRange WRITE setShowWeaponRange NOTIFY showWeaponRangeChanged)
	Q_PROPERTY(bool showFormationRange READ showFormationRange WRITE setShowFormationRange NOTIFY showFormationRangeChanged)
	Q_PROPERTY(bool showRoute READ showRoute WRITE setShowRoute NOTIFY showRouteChanged)
	Q_PROPERTY(bool showPresetRoute READ showPresetRoute WRITE setShowPresetRoute NOTIFY showPresetRouteChanged)
	Q_PROPERTY(bool showActualRoute READ showActualRoute WRITE setShowActualRoute NOTIFY showActualRouteChanged)
	Q_PROPERTY(QString labelFontFamily READ labelFontFamily WRITE setLabelFontFamily NOTIFY labelFontFamilyChanged)
	Q_PROPERTY(int labelPixelSize READ labelPixelSize WRITE setLabelPixelSize NOTIFY labelPixelSizeChanged)
	Q_PROPERTY(qreal iconScale READ iconScale WRITE setIconScale NOTIFY iconScaleChanged)
	Q_PROPERTY(bool useSourceModels READ useSourceModels WRITE setUseSourceModels NOTIFY useSourceModelsChanged)

public:
	explicit UiOsgInitMap(QQuickItem* parent = nullptr);
	~UiOsgInitMap() override;

	QWindow* mapWindow() const;

	QVariantList platformRows() const;
	void setPlatformRows(const QVariantList& rows);

	QString selectedPlatformId() const;
	void setSelectedPlatformId(const QString& platform_id);

	bool showPlatformId() const;
	void setShowPlatformId(bool visible);

	bool showCommandLink() const;
	void setShowCommandLink(bool visible);

	bool showSensorRange() const;
	void setShowSensorRange(bool visible);

	bool showWeaponRange() const;
	void setShowWeaponRange(bool visible);

	bool showFormationRange() const;
	void setShowFormationRange(bool visible);

	bool showRoute() const;
	void setShowRoute(bool visible);
	bool showPresetRoute() const;
	void setShowPresetRoute(bool visible);
	bool showActualRoute() const;
	void setShowActualRoute(bool visible);

	QString labelFontFamily() const;
	void setLabelFontFamily(const QString& family);

	int labelPixelSize() const;
	void setLabelPixelSize(int pixel_size);

	qreal iconScale() const;
	void setIconScale(qreal scale);

	bool useSourceModels() const;
	void setUseSourceModels(bool enabled);

	Q_INVOKABLE QString platformAt(qreal x, qreal y) const;
	Q_INVOKABLE QVariantMap geoAt(qreal x, qreal y) const;
	Q_INVOKABLE double lonAt(qreal x) const;
	Q_INVOKABLE double latAt(qreal y) const;
	Q_INVOKABLE void requestRedraw();

signals:
	void platformRowsChanged();
	void selectedPlatformIdChanged();
	void showPlatformIdChanged();
	void showCommandLinkChanged();
	void showSensorRangeChanged();
	void showWeaponRangeChanged();
	void showFormationRangeChanged();
	void showRouteChanged();
	void showPresetRouteChanged();
	void showActualRouteChanged();
	void labelFontFamilyChanged();
	void labelPixelSizeChanged();
	void iconScaleChanged();
	void useSourceModelsChanged();
	void platformClicked(const QString& platform_id);
	void platformRightClicked(const QString& platform_id);

protected:
	void geometryChange(const QRectF& new_geometry, const QRectF& old_geometry) override;
	void itemChange(ItemChange change, const ItemChangeData& value) override;

private:
	friend class UiOsgInitMapWindow;

	struct RoutePoint {
		double lon_deg = 0.0;
		double lat_deg = 0.0;
		double altitude_m = 0.0;
		double speed_mps = 0.0;
	};

	struct Platform {
		QString id;
		QString commander;
		QString side;
		QString icon;
		QString entity_kind;
		double lon_deg = 0.0;
		double lat_deg = 0.0;
		double altitude_m = 0.0;
		double speed_mps = 0.0;
		double heading_deg = 0.0;
		double path_angle_deg = 0.0;
		double max_sensor_range_m = 0.0;
		double max_weapon_range_m = 0.0;
		double formation_range_m = 0.0;
		QVector<RoutePoint> route_points;
		QVector<RoutePoint> trail_points;
	};

	struct Bounds {
		double min_lon = 128.0;
		double max_lon = 137.0;
		double min_lat = 37.6;
		double max_lat = 40.2;
	};

	struct MapProjection {
		double center_lon = 0.0;
		double center_lat = 0.0;
		double meters_per_deg_lon = 1.0;
		double meters_per_deg_lat = 1.0;
		qreal scene_center_x = 0.0;
		qreal scene_center_y = 0.0;
		qreal scene_pixels_per_meter = 1.0;
	};

	struct SharedState;

	void rebuildBounds();
	bool samePlatforms(const QVector<Platform>& next_platforms) const;
	MapProjection currentProjection() const;
	QPointF mapPoint(double lon_deg, double lat_deg) const;
	QVariantMap geoAtFallback(qreal x, qreal y) const;
	bool cameraGeoAt(qreal x, qreal y, double& lon_deg, double& lat_deg) const;
	QString cameraPlatformAt(qreal x, qreal y) const;
	UiOsgInitMapWindow* ensureMapWindow() const;
	void syncMapWindowScreen() const;
	void syncWindowScene();
	void emitWindowPlatformClicked(const QString& platform_id);
	void emitWindowPlatformRightClicked(const QString& platform_id);

	QVector<Platform> platforms;
	QHash<QString, int> platform_index;
	Bounds scene_bounds;
	QString selected_platform_id;
	bool draw_platform_id = false;
	bool draw_command_link = true;
	bool draw_sensor_range = false;
	bool draw_weapon_range = false;
	bool draw_formation_range = true;
	bool draw_route = true;
	bool draw_preset_route = true;
	bool draw_actual_route = true;
	QString label_font_family = QStringLiteral("Microsoft YaHei UI");
	int label_pixel_size = 13;
	qreal icon_scale = 1.0;
	bool use_source_models = true;
	qreal view_scale = 1.0;
	QPointF view_offset;
	quint64 scene_revision = 1;
	std::shared_ptr<SharedState> shared_state;
	mutable std::unique_ptr<UiOsgInitMapWindow> map_window;
};
