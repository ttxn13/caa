#pragma once

#include "pch.hpp"

#include <QHash>
#include <QPainterPath>
#include <QPointF>
#include <QQuickItem>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

class UiInitMap : public QQuickItem
{
	Q_OBJECT

	Q_PROPERTY(QVariantList platformRows READ platformRows WRITE setPlatformRows NOTIFY platformRowsChanged)
	Q_PROPERTY(QString selectedPlatformId READ selectedPlatformId WRITE setSelectedPlatformId NOTIFY selectedPlatformIdChanged)
	Q_PROPERTY(bool showPlatformId READ showPlatformId WRITE setShowPlatformId NOTIFY showPlatformIdChanged)
	Q_PROPERTY(bool showCommandLink READ showCommandLink WRITE setShowCommandLink NOTIFY showCommandLinkChanged)
	Q_PROPERTY(bool showSensorRange READ showSensorRange WRITE setShowSensorRange NOTIFY showSensorRangeChanged)
	Q_PROPERTY(bool showWeaponRange READ showWeaponRange WRITE setShowWeaponRange NOTIFY showWeaponRangeChanged)
	Q_PROPERTY(bool showRoute READ showRoute WRITE setShowRoute NOTIFY showRouteChanged)
	Q_PROPERTY(QString labelFontFamily READ labelFontFamily WRITE setLabelFontFamily NOTIFY labelFontFamilyChanged)
	Q_PROPERTY(int labelPixelSize READ labelPixelSize WRITE setLabelPixelSize NOTIFY labelPixelSizeChanged)
	Q_PROPERTY(qreal iconScale READ iconScale WRITE setIconScale NOTIFY iconScaleChanged)

public:
	explicit UiInitMap(QQuickItem* parent = nullptr);

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

	bool showRoute() const;
	void setShowRoute(bool visible);

	QString labelFontFamily() const;
	void setLabelFontFamily(const QString& family);

	int labelPixelSize() const;
	void setLabelPixelSize(int pixel_size);

	qreal iconScale() const;
	void setIconScale(qreal scale);

	Q_INVOKABLE QString platformAt(qreal x, qreal y) const;
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
	void showRouteChanged();
	void labelFontFamilyChanged();
	void labelPixelSizeChanged();
	void iconScaleChanged();
	void platformClicked(const QString& platform_id);
	void platformRightClicked(const QString& platform_id);

protected:
	QSGNode* updatePaintNode(QSGNode* old_node, UpdatePaintNodeData* update_data) override;
	void geometryChange(const QRectF& new_geometry, const QRectF& old_geometry) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

private:
	struct Platform {
		QString id;
		QString commander;
		QString side;
		QString icon;
		QString entity_kind;
		double lon_deg = 0.0;
		double lat_deg = 0.0;
		double heading_deg = 0.0;
		double max_sensor_range_m = 0.0;
		double max_weapon_range_m = 0.0;
		QVector<QPointF> route_points;
	};

	struct IconShape {
		QPainterPath path;
		QPainterPath outline_path;
		bool valid = false;
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

	void rebuildBounds();
	bool samePlatforms(const QVector<Platform>& next_platforms) const;
	MapProjection currentProjection() const;
	qreal mapLon(double lon_deg) const;
	qreal mapLat(double lat_deg) const;
	QPointF mapPoint(double lon_deg, double lat_deg) const;
	qreal rangePixels(const Platform& platform, double range_m) const;
	void drawScaleBar(QPainter& painter, const MapProjection& projection,
		const QFontMetricsF& metrics, qreal item_width, qreal item_height) const;
	const IconShape& iconShape(const QString& icon);
	void invalidateView();

	QVector<Platform> platforms;
	QHash<QString, int> platform_index;
	QHash<QString, IconShape> icon_shape_cache;
	Bounds scene_bounds;
	QString selected_platform_id;
	bool draw_platform_id = false;
	bool draw_command_link = true;
	bool draw_sensor_range = false;
	bool draw_weapon_range = false;
	bool draw_route = true;
	QString label_font_family = QStringLiteral("Microsoft YaHei UI");
	int label_pixel_size = 13;
	qreal icon_scale = 1.0;
	qreal view_scale = 1.0;
	QPointF view_offset;
	QPointF last_pan_pos;
	bool panning = false;
	quint64 geometry_revision = 1;
	quint64 vector_revision = 1;
	quint64 selection_revision = 1;
	quint64 overlay_revision = 1;
};
