#include "pch.hpp"
#include "UiInitMap.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontMetricsF>
#include <QMouseEvent>
#include <QPainter>
#include <QRegularExpression>
#include <QSGFlatColorMaterial>
#include <QSGGeometryNode>
#include <QSGSimpleTextureNode>
#include <QSGTexture>
#include <QQuickWindow>
#include <QWheelEvent>

namespace {
constexpr qreal kPlotPadding = 36.0;
constexpr qreal kGridStep = 72.0;
constexpr qreal kHitRadiusSquared = 484.0;
constexpr int kCircleSegments = 64;
constexpr qreal kIconBaseSize = 24.0;
constexpr double kMetersPerDegLat = 111320.0;

QColor side_color(const QString& side)
{
	if (side == QStringLiteral("red")) return QColor(QStringLiteral("#ff4d4d"));
	if (side == QStringLiteral("blue")) return QColor(QStringLiteral("#00bdff"));
	return QColor(QStringLiteral("#ffffff"));
}

QColor route_color(const QString& side, bool selected)
{
	if (side == QStringLiteral("red")) return selected ? QColor(255, 77, 77, 190) : QColor(255, 96, 96, 92);
	if (side == QStringLiteral("blue")) return selected ? QColor(0, 189, 255, 190) : QColor(35, 201, 255, 92);
	return selected ? QColor(255, 255, 255, 190) : QColor(255, 255, 255, 86);
}

QColor waypoint_color(const QString& side, bool selected)
{
	if (side == QStringLiteral("red")) return selected ? QColor(255, 77, 77, 230) : QColor(255, 96, 96, 160);
	if (side == QStringLiteral("blue")) return selected ? QColor(0, 189, 255, 230) : QColor(35, 201, 255, 160);
	return selected ? QColor(255, 255, 255, 220) : QColor(255, 255, 255, 150);
}

QColor command_link_color(const QString& side)
{
	if (side == QStringLiteral("red")) return QColor(255, 77, 77, 185);
	if (side == QStringLiteral("blue")) return QColor(0, 189, 255, 185);
	return QColor(255, 255, 255, 168);
}

QColor track_color(const QString& side)
{
	if (side == QStringLiteral("red")) return QColor(255, 77, 77, 130);
	if (side == QStringLiteral("blue")) return QColor(0, 189, 255, 130);
	return QColor(255, 255, 255, 118);
}

double map_double(const QVariantMap& map, const QString& key, double fallback = 0.0)
{
	const QVariant value = map.value(key);
	return value.isValid() ? value.toDouble() : fallback;
}

QString map_string(const QVariantMap& map, const QString& key)
{
	const QVariant value = map.value(key);
	return value.isValid() ? value.toString() : QString();
}

qreal nice_scale_distance_m(qreal target_m)
{
	if (target_m <= 0.0) return 0.0;
	const qreal exponent = std::floor(std::log10(target_m));
	const qreal base = std::pow(10.0, exponent);
	const qreal normalized = target_m / base;
	const qreal nice = normalized >= 5.0 ? 5.0 : (normalized >= 2.0 ? 2.0 : 1.0);
	return nice * base;
}

QString format_scale_distance(qreal distance_m)
{
	if (distance_m >= 1000.0) {
		const qreal distance_km = distance_m / 1000.0;
		return QStringLiteral("%1 km").arg(distance_km, 0, distance_km >= 10.0 ? 'f' : 'g', distance_km >= 10.0 ? 0 : 2);
	}
	return QStringLiteral("%1 m").arg(qRound(distance_m));
}

QString normalize_icon_key(const QString& icon)
{
	QString key = icon.trimmed();
	key.replace(QChar('\\'), QChar('/'));
	if (key.endsWith(QStringLiteral(".obj"), Qt::CaseInsensitive)) {
		key.chop(4);
	}
	const int slash = key.lastIndexOf(QChar('/'));
	if (slash >= 0) {
		key = key.mid(slash + 1);
	}
	return key.trimmed().toLower();
}

int parse_face_vertex_index(const QString& token, int vertex_count)
{
	QString index_text = token;
	const int slash = index_text.indexOf(QChar('/'));
	if (slash >= 0) {
		index_text.truncate(slash);
	}

	bool ok = false;
	int index = index_text.toInt(&ok);
	if (!ok || index == 0) return -1;
	if (index < 0) {
		index = vertex_count + index;
	}
	else {
		index -= 1;
	}
	return index >= 0 && index < vertex_count ? index : -1;
}

qreal polygon_area(const QVector<QPointF>& vertices, const QVector<int>& face)
{
	qreal area = 0.0;
	for (int i = 0; i < face.size(); ++i) {
		const QPointF& a = vertices[face[i]];
		const QPointF& b = vertices[face[(i + 1) % face.size()]];
		area += a.x() * b.y() - b.x() * a.y();
	}
	return area * 0.5;
}

QPainterPath fallback_aircraft_path()
{
	QPainterPath path;
	path.setFillRule(Qt::WindingFill);
	path.moveTo(0.0, -1.0);
	path.lineTo(0.16, -0.18);
	path.lineTo(0.88, 0.16);
	path.lineTo(0.22, 0.30);
	path.lineTo(0.17, 0.72);
	path.lineTo(0.46, 0.90);
	path.lineTo(0.08, 0.86);
	path.lineTo(0.0, 1.0);
	path.lineTo(-0.08, 0.86);
	path.lineTo(-0.46, 0.90);
	path.lineTo(-0.17, 0.72);
	path.lineTo(-0.22, 0.30);
	path.lineTo(-0.88, 0.16);
	path.lineTo(-0.16, -0.18);
	path.closeSubpath();
	return path;
}

QPainterPath fallback_missile_path()
{
	QPainterPath path;
	path.setFillRule(Qt::WindingFill);
	path.moveTo(0.0, -1.0);
	path.lineTo(0.34, -0.18);
	path.lineTo(0.22, 0.52);
	path.lineTo(0.48, 0.86);
	path.lineTo(0.10, 0.70);
	path.lineTo(0.0, 1.0);
	path.lineTo(-0.10, 0.70);
	path.lineTo(-0.48, 0.86);
	path.lineTo(-0.22, 0.52);
	path.lineTo(-0.34, -0.18);
	path.closeSubpath();
	return path;
}

QString mesh_dir_path()
{
	const QDir app_dir(QCoreApplication::applicationDirPath());
	const QStringList candidates = {
		app_dir.filePath(QStringLiteral("App/mesh")),
		app_dir.filePath(QStringLiteral("../App/mesh")),
		app_dir.filePath(QStringLiteral("mesh")),
		QDir::current().filePath(QStringLiteral("ICGAS/App/mesh")),
		QDir::current().filePath(QStringLiteral("App/mesh"))
	};
	for (const QString& candidate : candidates) {
		if (QDir(candidate).exists()) return QDir(candidate).absolutePath();
	}
	return QDir::current().filePath(QStringLiteral("ICGAS/App/mesh"));
}

QString mesh_path_for_icon(const QString& icon)
{
	const QString key = normalize_icon_key(icon);
	const QDir mesh_dir(mesh_dir_path());
	if (key.isEmpty()) return mesh_dir.filePath(QStringLiteral("Core.FixedWing.obj"));

	QString direct = icon.trimmed();
	direct.replace(QChar('\\'), QChar('/'));
	if (direct.endsWith(QStringLiteral(".obj"), Qt::CaseInsensitive)) {
		const QFileInfo direct_info(direct);
		if (direct_info.isAbsolute() && direct_info.exists()) return direct_info.absoluteFilePath();
		const QFileInfo mesh_relative(mesh_dir.filePath(direct));
		if (mesh_relative.exists()) return mesh_relative.absoluteFilePath();
	}

	const QFileInfo exact(mesh_dir.filePath(key + QStringLiteral(".obj")));
	if (exact.exists()) return exact.absoluteFilePath();

	const QFileInfo fixed_wing(mesh_dir.filePath(QStringLiteral("FixedWing.") + key + QStringLiteral(".obj")));
	if (fixed_wing.exists()) return fixed_wing.absoluteFilePath();

	const QFileInfo rotorcraft(mesh_dir.filePath(QStringLiteral("Rotorcraft.") + key + QStringLiteral(".obj")));
	if (rotorcraft.exists()) return rotorcraft.absoluteFilePath();

	const QFileInfo watercraft(mesh_dir.filePath(QStringLiteral("Watercraft.") + key + QStringLiteral(".obj")));
	if (watercraft.exists()) return watercraft.absoluteFilePath();

	const QFileInfo missile(mesh_dir.filePath(QStringLiteral("Missile.") + key + QStringLiteral(".obj")));
	if (missile.exists()) return missile.absoluteFilePath();

	const QFileInfo vehicle(mesh_dir.filePath(QStringLiteral("Vehicle.") + key + QStringLiteral(".obj")));
	if (vehicle.exists()) return vehicle.absoluteFilePath();

	const QStringList files = mesh_dir.entryList(QStringList() << QStringLiteral("*.obj"), QDir::Files);
	QString best;
	int best_score = -1;
	for (const QString& file : files) {
		const QString stem = QFileInfo(file).completeBaseName();
		const QString normalized_stem = normalize_icon_key(stem);
		int score = -1;
		if (normalized_stem == key) score = 1000;
		else if (normalized_stem.endsWith(QStringLiteral(".") + key)) score = 900;
		else if (normalized_stem.contains(key)) score = 700;
		else if (key.contains(normalized_stem)) score = 500;
		if (score > best_score) {
			best_score = score;
			best = file;
		}
	}
	if (best_score >= 0) return mesh_dir.filePath(best);

	if (key == QStringLiteral("ea-18g") || key == QStringLiteral("ea18g")) {
		const QFileInfo fallback(mesh_dir.filePath(QStringLiteral("FixedWing.F-18E.obj")));
		if (fallback.exists()) return fallback.absoluteFilePath();
	}
	if (key == QStringLiteral("ddg-51") || key == QStringLiteral("ddg51")) {
		const QFileInfo fallback(mesh_dir.filePath(QStringLiteral("Watercraft.Type 052D.obj")));
		if (fallback.exists()) return fallback.absoluteFilePath();
	}
	if (key.contains(QStringLiteral("cvn")) || key.contains(QStringLiteral("ddg")) ||
		key.contains(QStringLiteral("cg-")) || key.contains(QStringLiteral("frigate")) ||
		key.contains(QStringLiteral("car")) || key.contains(QStringLiteral("ship"))) {
		return mesh_dir.filePath(QStringLiteral("Core.Watercraft.obj"));
	}
	if (key.contains(QStringLiteral("sam")) || key.contains(QStringLiteral("truck")) || key.contains(QStringLiteral("tank"))) {
		return mesh_dir.filePath(QStringLiteral("Core.Vehicle.obj"));
	}
	return mesh_dir.filePath(QStringLiteral("Core.FixedWing.obj"));
}

qreal cross(const QPointF& origin, const QPointF& a, const QPointF& b)
{
	return (a.x() - origin.x()) * (b.y() - origin.y()) - (a.y() - origin.y()) * (b.x() - origin.x());
}

QVector<QPointF> convex_hull(QVector<QPointF> points)
{
	if (points.size() <= 3) return points;

	std::sort(points.begin(), points.end(), [](const QPointF& a, const QPointF& b) {
		return a.x() == b.x() ? a.y() < b.y() : a.x() < b.x();
	});
	points.erase(std::unique(points.begin(), points.end(), [](const QPointF& a, const QPointF& b) {
		return std::abs(a.x() - b.x()) < 0.0001 && std::abs(a.y() - b.y()) < 0.0001;
	}), points.end());
	if (points.size() <= 3) return points;

	QVector<QPointF> lower;
	for (const QPointF& point : points) {
		while (lower.size() >= 2 && cross(lower[lower.size() - 2], lower.last(), point) <= 0.0) {
			lower.removeLast();
		}
		lower.push_back(point);
	}

	QVector<QPointF> upper;
	for (int i = points.size() - 1; i >= 0; --i) {
		const QPointF& point = points[i];
		while (upper.size() >= 2 && cross(upper[upper.size() - 2], upper.last(), point) <= 0.0) {
			upper.removeLast();
		}
		upper.push_back(point);
	}

	lower.removeLast();
	upper.removeLast();
	lower += upper;
	return lower;
}

QPainterPath closed_path_from_points(const QVector<QPointF>& points)
{
	QPainterPath path;
	if (points.size() < 3) return path;

	path.moveTo(points[0]);
	for (int i = 1; i < points.size(); ++i) {
		path.lineTo(points[i]);
	}
	path.closeSubpath();
	return path;
}

QSGGeometryNode* make_lines_node(const QVector<QLineF>& lines, const QColor& color)
{
	if (lines.isEmpty()) return nullptr;

	auto* geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), lines.size() * 2);
	geometry->setDrawingMode(QSGGeometry::DrawLines);
	QSGGeometry::Point2D* vertices = geometry->vertexDataAsPoint2D();
	for (int i = 0; i < lines.size(); ++i) {
		vertices[i * 2].set(static_cast<float>(lines[i].p1().x()), static_cast<float>(lines[i].p1().y()));
		vertices[i * 2 + 1].set(static_cast<float>(lines[i].p2().x()), static_cast<float>(lines[i].p2().y()));
	}

	auto* material = new QSGFlatColorMaterial();
	material->setColor(color);
	material->setFlag(QSGMaterial::Blending, color.alpha() < 255);

	auto* node = new QSGGeometryNode();
	node->setGeometry(geometry);
	node->setFlag(QSGNode::OwnsGeometry);
	node->setMaterial(material);
	node->setFlag(QSGNode::OwnsMaterial);
	return node;
}

void append_thick_line(QVector<QPointF>& triangles, const QLineF& line, qreal width)
{
	const QPointF p1 = line.p1();
	const QPointF p2 = line.p2();
	const qreal dx = p2.x() - p1.x();
	const qreal dy = p2.y() - p1.y();
	const qreal length = std::hypot(dx, dy);
	if (length <= 0.01) return;

	const qreal half_width = width * 0.5;
	const QPointF normal(-dy / length * half_width, dx / length * half_width);
	const QPointF a(p1.x() + normal.x(), p1.y() + normal.y());
	const QPointF b(p1.x() - normal.x(), p1.y() - normal.y());
	const QPointF c(p2.x() + normal.x(), p2.y() + normal.y());
	const QPointF d(p2.x() - normal.x(), p2.y() - normal.y());

	triangles.push_back(a);
	triangles.push_back(b);
	triangles.push_back(c);
	triangles.push_back(c);
	triangles.push_back(b);
	triangles.push_back(d);
}

void append_ring(QVector<QPointF>& triangles, const QPointF& center, qreal outer_radius, qreal inner_radius)
{
	for (int i = 0; i < kCircleSegments; ++i) {
		const qreal angle_a = (static_cast<qreal>(i) / kCircleSegments) * 2.0 * std::numbers::pi_v<qreal>;
		const qreal angle_b = (static_cast<qreal>(i + 1) / kCircleSegments) * 2.0 * std::numbers::pi_v<qreal>;
		const QPointF outer_a(center.x() + std::cos(angle_a) * outer_radius, center.y() + std::sin(angle_a) * outer_radius);
		const QPointF outer_b(center.x() + std::cos(angle_b) * outer_radius, center.y() + std::sin(angle_b) * outer_radius);
		const QPointF inner_a(center.x() + std::cos(angle_a) * inner_radius, center.y() + std::sin(angle_a) * inner_radius);
		const QPointF inner_b(center.x() + std::cos(angle_b) * inner_radius, center.y() + std::sin(angle_b) * inner_radius);
		triangles.push_back(outer_a);
		triangles.push_back(inner_a);
		triangles.push_back(outer_b);
		triangles.push_back(outer_b);
		triangles.push_back(inner_a);
		triangles.push_back(inner_b);
	}
}

void append_disc(QVector<QPointF>& triangles, const QPointF& center, qreal radius)
{
	for (int i = 0; i < kCircleSegments; ++i) {
		const qreal angle_a = (static_cast<qreal>(i) / kCircleSegments) * 2.0 * std::numbers::pi_v<qreal>;
		const qreal angle_b = (static_cast<qreal>(i + 1) / kCircleSegments) * 2.0 * std::numbers::pi_v<qreal>;
		triangles.push_back(center);
		triangles.push_back(QPointF(center.x() + std::cos(angle_a) * radius, center.y() + std::sin(angle_a) * radius));
		triangles.push_back(QPointF(center.x() + std::cos(angle_b) * radius, center.y() + std::sin(angle_b) * radius));
	}
}

void append_arrow_head(QVector<QPointF>& triangles, const QLineF& line, qreal length, qreal width)
{
	const QPointF p1 = line.p1();
	const QPointF p2 = line.p2();
	const qreal dx = p2.x() - p1.x();
	const qreal dy = p2.y() - p1.y();
	const qreal line_length = std::hypot(dx, dy);
	if (line_length <= 0.01) return;

	const QPointF unit(dx / line_length, dy / line_length);
	const QPointF normal(-unit.y(), unit.x());
	const QPointF base(p2.x() - unit.x() * length, p2.y() - unit.y() * length);
	triangles.push_back(p2);
	triangles.push_back(QPointF(base.x() + normal.x() * width, base.y() + normal.y() * width));
	triangles.push_back(QPointF(base.x() - normal.x() * width, base.y() - normal.y() * width));
}

QSGGeometryNode* make_triangles_node(const QVector<QPointF>& points, const QColor& color)
{
	if (points.isEmpty()) return nullptr;

	auto* geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), points.size());
	geometry->setDrawingMode(QSGGeometry::DrawTriangles);
	QSGGeometry::Point2D* vertices = geometry->vertexDataAsPoint2D();
	for (int i = 0; i < points.size(); ++i) {
		vertices[i].set(static_cast<float>(points[i].x()), static_cast<float>(points[i].y()));
	}

	auto* material = new QSGFlatColorMaterial();
	material->setColor(color);
	material->setFlag(QSGMaterial::Blending, color.alpha() < 255);

	auto* node = new QSGGeometryNode();
	node->setGeometry(geometry);
	node->setFlag(QSGNode::OwnsGeometry);
	node->setMaterial(material);
	node->setFlag(QSGNode::OwnsMaterial);
	return node;
}

QSGGeometryNode* make_thick_lines_node(const QVector<QLineF>& lines, const QColor& color, qreal width)
{
	if (lines.isEmpty()) return nullptr;

	QVector<QPointF> triangles;
	triangles.reserve(lines.size() * 6);
	for (const QLineF& line : lines) {
		append_thick_line(triangles, line, width);
	}
	return make_triangles_node(triangles, color);
}

QSGGeometryNode* make_track_node(const QVector<QLineF>& lines, const QColor& color)
{
	if (lines.isEmpty()) return nullptr;

	QVector<QPointF> triangles;
	triangles.reserve(lines.size() * 9);
	for (const QLineF& line : lines) {
		append_thick_line(triangles, line, 2.0);
		append_arrow_head(triangles, line, 8.0, 4.0);
	}
	return make_triangles_node(triangles, color);
}

class MapRootNode : public QSGNode
{
public:
	~MapRootNode() override
	{
		clearLayer(grid_layer);
		clearLayer(vector_layer);
		clearLayer(selection_layer);
		clearOverlay();
	}

	void clearLayer(QSGNode*& layer)
	{
		if (layer == nullptr) return;
		removeChildNode(layer);
		delete layer;
		layer = nullptr;
	}

	void clearOverlay()
	{
		if (overlay_node == nullptr) return;
		removeChildNode(overlay_node);
		delete overlay_node;
		overlay_node = nullptr;
	}

	void appendLayerAfter(QSGNode* layer, QSGNode* previous)
	{
		if (layer == nullptr) return;
		bool previous_is_child = false;
		for (QSGNode* child = firstChild(); child != nullptr; child = child->nextSibling()) {
			if (child == previous) {
				previous_is_child = true;
				break;
			}
		}
		if (previous_is_child) {
			insertChildNodeAfter(layer, previous);
		}
		else {
			prependChildNode(layer);
		}
	}

	QSGNode* grid_layer = nullptr;
	QSGNode* vector_layer = nullptr;
	QSGNode* selection_layer = nullptr;
	QSGSimpleTextureNode* overlay_node = nullptr;
	quint64 grid_revision = 0;
	quint64 vector_revision = 0;
	quint64 selection_revision = 0;
	quint64 overlay_revision = 0;
	qreal overlay_device_ratio = 0.0;
};
}

UiInitMap::UiInitMap(QQuickItem* parent)
	: QQuickItem(parent)
{
	setFlag(ItemHasContents, true);
	setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
	setAcceptHoverEvents(true);
	setAcceptTouchEvents(false);
	setFiltersChildMouseEvents(false);
}

QVariantList UiInitMap::platformRows() const
{
	QVariantList rows;
	rows.reserve(platforms.size());
	for (const Platform& platform : platforms) {
		QVariantMap row;
		row[QStringLiteral("platform_id")] = platform.id;
		row[QStringLiteral("commander")] = platform.commander;
		row[QStringLiteral("side")] = platform.side;
		row[QStringLiteral("icon")] = platform.icon;
		row[QStringLiteral("lon_deg")] = platform.lon_deg;
		row[QStringLiteral("lat_deg")] = platform.lat_deg;
		row[QStringLiteral("heading_deg")] = platform.heading_deg;
		row[QStringLiteral("max_sensor_range_m")] = platform.max_sensor_range_m;
		row[QStringLiteral("max_weapon_range_m")] = platform.max_weapon_range_m;
		QVariantList route_points;
		route_points.reserve(platform.route_points.size());
		for (const QPointF& point : platform.route_points) {
			QVariantMap route_point;
			route_point[QStringLiteral("lon_deg")] = point.x();
			route_point[QStringLiteral("lat_deg")] = point.y();
			route_points.push_back(route_point);
		}
		row[QStringLiteral("route_points")] = route_points;
		rows.push_back(row);
	}
	return rows;
}

void UiInitMap::setPlatformRows(const QVariantList& rows)
{
	QVector<Platform> next_platforms;
	QHash<QString, int> next_platform_index;
	next_platforms.reserve(rows.size());

	for (const QVariant& row_value : rows) {
		const QVariantMap row = row_value.toMap();
		Platform platform;
		platform.id = map_string(row, QStringLiteral("platform_id"));
		platform.commander = map_string(row, QStringLiteral("commander"));
		platform.side = map_string(row, QStringLiteral("side"));
		platform.icon = map_string(row, QStringLiteral("icon"));
		platform.entity_kind = map_string(row, QStringLiteral("entity_kind"));
		platform.lon_deg = map_double(row, QStringLiteral("lon_deg"));
		platform.lat_deg = map_double(row, QStringLiteral("lat_deg"));
		platform.heading_deg = map_double(row, QStringLiteral("heading_deg"));
		platform.max_sensor_range_m = map_double(row, QStringLiteral("max_sensor_range_m"));
		platform.max_weapon_range_m = map_double(row, QStringLiteral("max_weapon_range_m"));

		const QVariantList route_points = row.value(QStringLiteral("route_points")).toList();
		platform.route_points.reserve(route_points.size());
		for (const QVariant& route_value : route_points) {
			const QVariantMap route_point = route_value.toMap();
			platform.route_points.push_back(QPointF(
				map_double(route_point, QStringLiteral("lon_deg")),
				map_double(route_point, QStringLiteral("lat_deg"))));
		}

		if (!platform.id.isEmpty()) {
			next_platform_index.insert(platform.id, next_platforms.size());
		}
		next_platforms.push_back(platform);
	}

	if (samePlatforms(next_platforms)) {
		return;
	}

	const bool had_platforms = !platforms.isEmpty();
	const MapProjection old_projection = currentProjection();
	const qreal old_view_scale = view_scale;
	const QPointF old_view_offset = view_offset;
	const QPointF viewport_center(width() * 0.5, height() * 0.5);
	const qreal old_scene_x = (viewport_center.x() - old_view_offset.x()) / std::max<qreal>(0.01, old_view_scale);
	const qreal old_scene_y = (viewport_center.y() - old_view_offset.y()) / std::max<qreal>(0.01, old_view_scale);
	const double old_center_lon = old_projection.center_lon +
		(old_scene_x - old_projection.scene_center_x) /
		std::max<qreal>(0.000001, old_projection.scene_pixels_per_meter) /
		old_projection.meters_per_deg_lon;
	const double old_center_lat = old_projection.center_lat -
		(old_scene_y - old_projection.scene_center_y) /
		std::max<qreal>(0.000001, old_projection.scene_pixels_per_meter) /
		old_projection.meters_per_deg_lat;
	const qreal old_pixels_per_meter = old_projection.scene_pixels_per_meter * old_view_scale;

	platforms = std::move(next_platforms);
	platform_index = std::move(next_platform_index);

	for (const Platform& platform : platforms) {
		iconShape(platform.icon);
	}

	rebuildBounds();
	if (had_platforms && old_pixels_per_meter > 0.0) {
		const MapProjection new_projection = currentProjection();
		view_scale = std::clamp(old_pixels_per_meter /
			std::max<qreal>(0.000001, new_projection.scene_pixels_per_meter),
			static_cast<qreal>(0.45), static_cast<qreal>(8.0));
		const qreal new_scene_x = new_projection.scene_center_x +
			(old_center_lon - new_projection.center_lon) *
			new_projection.meters_per_deg_lon * new_projection.scene_pixels_per_meter;
		const qreal new_scene_y = new_projection.scene_center_y -
			(old_center_lat - new_projection.center_lat) *
			new_projection.meters_per_deg_lat * new_projection.scene_pixels_per_meter;
		view_offset = viewport_center - QPointF(new_scene_x * view_scale, new_scene_y * view_scale);
	}
	panning = false;
	++vector_revision;
	++selection_revision;
	++overlay_revision;
	update();
	emit platformRowsChanged();
}

QString UiInitMap::selectedPlatformId() const
{
	return selected_platform_id;
}

void UiInitMap::setSelectedPlatformId(const QString& platform_id)
{
	if (selected_platform_id == platform_id) return;
	selected_platform_id = platform_id;
	++selection_revision;
	++overlay_revision;
	update();
	emit selectedPlatformIdChanged();
}

bool UiInitMap::showPlatformId() const
{
	return draw_platform_id;
}

void UiInitMap::setShowPlatformId(bool visible)
{
	if (draw_platform_id == visible) return;
	draw_platform_id = visible;
	++overlay_revision;
	update();
	emit showPlatformIdChanged();
}

bool UiInitMap::showCommandLink() const
{
	return draw_command_link;
}

void UiInitMap::setShowCommandLink(bool visible)
{
	if (draw_command_link == visible) return;
	draw_command_link = visible;
	++vector_revision;
	update();
	emit showCommandLinkChanged();
}

bool UiInitMap::showSensorRange() const
{
	return draw_sensor_range;
}

void UiInitMap::setShowSensorRange(bool visible)
{
	if (draw_sensor_range == visible) return;
	draw_sensor_range = visible;
	++vector_revision;
	update();
	emit showSensorRangeChanged();
}

bool UiInitMap::showWeaponRange() const
{
	return draw_weapon_range;
}

void UiInitMap::setShowWeaponRange(bool visible)
{
	if (draw_weapon_range == visible) return;
	draw_weapon_range = visible;
	++vector_revision;
	update();
	emit showWeaponRangeChanged();
}

bool UiInitMap::showRoute() const
{
	return draw_route;
}

void UiInitMap::setShowRoute(bool visible)
{
	if (draw_route == visible) return;
	draw_route = visible;
	++vector_revision;
	++selection_revision;
	update();
	emit showRouteChanged();
}

QString UiInitMap::labelFontFamily() const
{
	return label_font_family;
}

void UiInitMap::setLabelFontFamily(const QString& family)
{
	if (label_font_family == family) return;
	label_font_family = family;
	++overlay_revision;
	update();
	emit labelFontFamilyChanged();
}

int UiInitMap::labelPixelSize() const
{
	return label_pixel_size;
}

void UiInitMap::setLabelPixelSize(int pixel_size)
{
	const int normalized_size = std::max(1, pixel_size);
	if (label_pixel_size == normalized_size) return;
	label_pixel_size = normalized_size;
	++overlay_revision;
	update();
	emit labelPixelSizeChanged();
}

qreal UiInitMap::iconScale() const
{
	return icon_scale;
}

void UiInitMap::setIconScale(qreal scale)
{
	const qreal normalized_scale = std::clamp(scale, qreal(0.35), qreal(2.0));
	if (qFuzzyCompare(icon_scale, normalized_scale)) return;
	icon_scale = normalized_scale;
	++overlay_revision;
	update();
	emit iconScaleChanged();
}

QString UiInitMap::platformAt(qreal x, qreal y) const
{
	QString best_id;
	qreal best_distance_squared = kHitRadiusSquared;
	for (const Platform& platform : platforms) {
		const QPointF point = mapPoint(platform.lon_deg, platform.lat_deg);
		const qreal dx = point.x() - x;
		const qreal dy = point.y() - y;
		const qreal distance_squared = dx * dx + dy * dy;
		if (distance_squared < best_distance_squared) {
			best_distance_squared = distance_squared;
			best_id = platform.id;
		}
	}
	return best_id;
}

double UiInitMap::lonAt(qreal x) const
{
	const MapProjection projection = currentProjection();
	const qreal scene_x = (x - view_offset.x()) / std::max<qreal>(0.01, view_scale);
	const double lon = projection.center_lon +
		(scene_x - projection.scene_center_x) /
		std::max<qreal>(0.000001, projection.scene_pixels_per_meter) /
		projection.meters_per_deg_lon;
	return std::clamp(lon, scene_bounds.min_lon, scene_bounds.max_lon);
}

double UiInitMap::latAt(qreal y) const
{
	const MapProjection projection = currentProjection();
	const qreal scene_y = (y - view_offset.y()) / std::max<qreal>(0.01, view_scale);
	const double lat = projection.center_lat -
		(scene_y - projection.scene_center_y) /
		std::max<qreal>(0.000001, projection.scene_pixels_per_meter) /
		projection.meters_per_deg_lat;
	return std::clamp(lat, scene_bounds.min_lat, scene_bounds.max_lat);
}

void UiInitMap::requestRedraw()
{
	++geometry_revision;
	++vector_revision;
	++selection_revision;
	++overlay_revision;
	update();
}

void UiInitMap::invalidateView()
{
	++geometry_revision;
	++vector_revision;
	++selection_revision;
	++overlay_revision;
	update();
}

QSGNode* UiInitMap::updatePaintNode(QSGNode* old_node, UpdatePaintNodeData*)
{
	MapRootNode* root = static_cast<MapRootNode*>(old_node);
	if (root == nullptr) {
		root = new MapRootNode();
	}

	const qreal item_width = width();
	const qreal item_height = height();
	if (item_width <= 1.0 || item_height <= 1.0) {
		root->clearLayer(root->grid_layer);
		root->clearLayer(root->vector_layer);
		root->clearLayer(root->selection_layer);
		root->clearOverlay();
		root->grid_revision = 0;
		root->vector_revision = 0;
		root->selection_revision = 0;
		root->overlay_revision = 0;
		return root;
	}

	if (root->grid_revision != geometry_revision) {
		root->clearLayer(root->grid_layer);
		root->grid_layer = new QSGNode();
		QVector<QLineF> grid_lines;
		const qreal grid_step = kGridStep * view_scale;
		const qreal grid_start_x = std::fmod(kPlotPadding * view_scale + view_offset.x(), grid_step);
		const qreal grid_start_y = std::fmod(kPlotPadding * view_scale + view_offset.y(), grid_step);
		for (qreal gx = grid_start_x; gx < item_width; gx += grid_step) {
			if (gx < 0.0) continue;
			grid_lines.push_back(QLineF(gx, kPlotPadding, gx, item_height - kPlotPadding));
		}
		for (qreal gy = grid_start_y; gy < item_height; gy += grid_step) {
			if (gy < 0.0) continue;
			grid_lines.push_back(QLineF(kPlotPadding, gy, item_width - kPlotPadding, gy));
		}
		if (QSGGeometryNode* grid = make_lines_node(grid_lines, QColor(QStringLiteral("#dbe5ef")))) {
			root->grid_layer->appendChildNode(grid);
		}
		root->prependChildNode(root->grid_layer);
		root->grid_revision = geometry_revision;
	}

	if (root->vector_revision != vector_revision) {
		root->clearLayer(root->vector_layer);
		root->vector_layer = new QSGNode();

		QVector<QLineF> red_track_lines;
		QVector<QLineF> blue_track_lines;
		QVector<QLineF> unknown_track_lines;
		for (const Platform& platform : platforms) {
			if (platform.entity_kind.isEmpty()) continue;
			if (platform.route_points.size() < 2) continue;
			for (int i = 1; i < platform.route_points.size(); ++i) {
				const QLineF segment(
					mapPoint(platform.route_points[i - 1].x(), platform.route_points[i - 1].y()),
					mapPoint(platform.route_points[i].x(), platform.route_points[i].y()));
				if (platform.side == QStringLiteral("red")) {
					red_track_lines.push_back(segment);
				}
				else if (platform.side == QStringLiteral("blue")) {
					blue_track_lines.push_back(segment);
				}
				else {
					unknown_track_lines.push_back(segment);
				}
			}
		}
		if (QSGGeometryNode* tracks = make_track_node(red_track_lines, track_color(QStringLiteral("red")))) {
			root->vector_layer->appendChildNode(tracks);
		}
		if (QSGGeometryNode* tracks = make_track_node(blue_track_lines, track_color(QStringLiteral("blue")))) {
			root->vector_layer->appendChildNode(tracks);
		}
		if (QSGGeometryNode* tracks = make_track_node(unknown_track_lines, track_color(QString()))) {
			root->vector_layer->appendChildNode(tracks);
		}

		if (draw_route) {
			QVector<QLineF> red_route_lines;
			QVector<QLineF> blue_route_lines;
			QVector<QLineF> unknown_route_lines;
			QVector<QPointF> red_waypoints;
			QVector<QPointF> blue_waypoints;
			QVector<QPointF> unknown_waypoints;
			for (const Platform& platform : platforms) {
				if (platform.route_points.isEmpty()) continue;
				for (const QPointF& route_point : platform.route_points) {
					const QPointF point = mapPoint(route_point.x(), route_point.y());
					if (platform.side == QStringLiteral("red")) {
						append_disc(red_waypoints, point, 2.4);
					}
					else if (platform.side == QStringLiteral("blue")) {
						append_disc(blue_waypoints, point, 2.4);
					}
					else {
						append_disc(unknown_waypoints, point, 2.4);
					}
				}
				if (platform.route_points.size() < 2) continue;
				for (int i = 1; i < platform.route_points.size(); ++i) {
					const QLineF segment(
						mapPoint(platform.route_points[i - 1].x(), platform.route_points[i - 1].y()),
						mapPoint(platform.route_points[i].x(), platform.route_points[i].y()));
					if (platform.side == QStringLiteral("red")) {
						red_route_lines.push_back(segment);
					}
					else if (platform.side == QStringLiteral("blue")) {
						blue_route_lines.push_back(segment);
					}
					else {
						unknown_route_lines.push_back(segment);
					}
				}
			}
			if (QSGGeometryNode* routes = make_lines_node(red_route_lines, route_color(QStringLiteral("red"), false))) {
				root->vector_layer->appendChildNode(routes);
			}
			if (QSGGeometryNode* routes = make_lines_node(blue_route_lines, route_color(QStringLiteral("blue"), false))) {
				root->vector_layer->appendChildNode(routes);
			}
			if (QSGGeometryNode* routes = make_lines_node(unknown_route_lines, route_color(QString(), false))) {
				root->vector_layer->appendChildNode(routes);
			}
			if (QSGGeometryNode* waypoints = make_triangles_node(red_waypoints, waypoint_color(QStringLiteral("red"), false))) {
				root->vector_layer->appendChildNode(waypoints);
			}
			if (QSGGeometryNode* waypoints = make_triangles_node(blue_waypoints, waypoint_color(QStringLiteral("blue"), false))) {
				root->vector_layer->appendChildNode(waypoints);
			}
			if (QSGGeometryNode* waypoints = make_triangles_node(unknown_waypoints, waypoint_color(QString(), false))) {
				root->vector_layer->appendChildNode(waypoints);
			}
		}

		if (draw_command_link) {
			QVector<QLineF> red_command_lines;
			QVector<QLineF> blue_command_lines;
			QVector<QLineF> unknown_command_lines;
			for (const Platform& platform : platforms) {
				if (platform.commander.isEmpty() || platform.commander == QStringLiteral("SELF")) continue;
				const auto commander_iter = platform_index.constFind(platform.commander);
				if (commander_iter == platform_index.constEnd()) continue;
				const Platform& commander = platforms[*commander_iter];
				const QLineF segment(
					mapPoint(platform.lon_deg, platform.lat_deg),
					mapPoint(commander.lon_deg, commander.lat_deg));
				if (platform.side == QStringLiteral("red")) {
					red_command_lines.push_back(segment);
				}
				else if (platform.side == QStringLiteral("blue")) {
					blue_command_lines.push_back(segment);
				}
				else {
					unknown_command_lines.push_back(segment);
				}
			}
			if (QSGGeometryNode* commands = make_lines_node(red_command_lines, command_link_color(QStringLiteral("red")))) {
				root->vector_layer->appendChildNode(commands);
			}
			if (QSGGeometryNode* commands = make_lines_node(blue_command_lines, command_link_color(QStringLiteral("blue")))) {
				root->vector_layer->appendChildNode(commands);
			}
			if (QSGGeometryNode* commands = make_lines_node(unknown_command_lines, command_link_color(QString()))) {
				root->vector_layer->appendChildNode(commands);
			}
		}

		if (draw_sensor_range || draw_weapon_range) {
			QVector<QPointF> sensor_rings;
			QVector<QPointF> weapon_rings;
			for (const Platform& platform : platforms) {
				const QPointF center = mapPoint(platform.lon_deg, platform.lat_deg);
				if (draw_sensor_range) {
					const qreal range = rangePixels(platform, platform.max_sensor_range_m);
					if (range > 2.0) append_ring(sensor_rings, center, range + 0.5, range - 0.5);
				}
				if (draw_weapon_range) {
					const qreal range = rangePixels(platform, platform.max_weapon_range_m);
					if (range > 2.0) append_ring(weapon_rings, center, range + 0.5, range - 0.5);
				}
			}
			if (QSGGeometryNode* sensors = make_triangles_node(sensor_rings, QColor(5, 150, 105, 46))) {
				root->vector_layer->appendChildNode(sensors);
			}
			if (QSGGeometryNode* weapons = make_triangles_node(weapon_rings, QColor(217, 119, 6, 51))) {
				root->vector_layer->appendChildNode(weapons);
			}
		}
		root->appendLayerAfter(root->vector_layer, root->grid_layer);
		root->vector_revision = vector_revision;
	}

	if (root->selection_revision != selection_revision || root->vector_revision != vector_revision) {
		root->clearLayer(root->selection_layer);
		root->selection_layer = new QSGNode();
		if (draw_route && !selected_platform_id.isEmpty()) {
			QVector<QLineF> selected_route_lines;
			QVector<QPointF> selected_waypoints;
			QString selected_route_side;
			for (const Platform& platform : platforms) {
				if (platform.id != selected_platform_id || platform.route_points.isEmpty()) continue;
				selected_route_side = platform.side;
				for (const QPointF& route_point : platform.route_points) {
					append_disc(selected_waypoints, mapPoint(route_point.x(), route_point.y()), 3.6);
				}
				if (platform.route_points.size() < 2) break;
				for (int i = 1; i < platform.route_points.size(); ++i) {
					selected_route_lines.push_back(QLineF(
						mapPoint(platform.route_points[i - 1].x(), platform.route_points[i - 1].y()),
						mapPoint(platform.route_points[i].x(), platform.route_points[i].y())));
				}
				break;
			}
			if (QSGGeometryNode* routes = make_thick_lines_node(selected_route_lines, route_color(selected_route_side, true), 3.5)) {
				root->selection_layer->appendChildNode(routes);
			}
			if (QSGGeometryNode* waypoints = make_triangles_node(selected_waypoints, waypoint_color(selected_route_side, true))) {
				root->selection_layer->appendChildNode(waypoints);
			}
		}
		root->appendLayerAfter(root->selection_layer, root->vector_layer != nullptr ? root->vector_layer : root->grid_layer);
		root->selection_revision = selection_revision;
	}

	const qreal device_ratio = window() == nullptr ? 1.0 : std::max<qreal>(1.0, window()->effectiveDevicePixelRatio());
	if (root->overlay_revision != overlay_revision || root->overlay_device_ratio != device_ratio) {
		root->clearOverlay();
		if (window() != nullptr) {
			QImage overlay_image(QSize(std::ceil(item_width * device_ratio), std::ceil(item_height * device_ratio)),
				QImage::Format_ARGB32_Premultiplied);
			overlay_image.fill(Qt::transparent);
			overlay_image.setDevicePixelRatio(device_ratio);

			QPainter painter(&overlay_image);
			painter.setRenderHint(QPainter::Antialiasing, true);
			painter.setRenderHint(QPainter::TextAntialiasing, true);
			QFont font(label_font_family);
			font.setPixelSize(label_pixel_size);
			painter.setFont(font);
			const QFontMetricsF metrics(font);
			const MapProjection projection = currentProjection();
			for (const Platform& platform : platforms) {
				const QPointF center = mapPoint(platform.lon_deg, platform.lat_deg);
				const bool selected = platform.id == selected_platform_id;
				const qreal radius = (selected ? kIconBaseSize * 0.58 : kIconBaseSize * 0.50) * icon_scale;
				const QColor fill_color = side_color(platform.side);
				const IconShape& shape = iconShape(platform.icon);

				painter.save();
				painter.translate(center);
				painter.rotate(platform.heading_deg);
				painter.scale(radius, radius);
				if (selected) {
					painter.save();
					painter.scale(1.15, 1.15);
					painter.setBrush(QColor(QStringLiteral("#111827")));
					painter.setPen(Qt::NoPen);
					painter.drawPath(shape.path);
					painter.restore();
				}
				else {
					painter.save();
					painter.scale(1.09, 1.09);
					painter.setBrush(QColor(255, 255, 255, 220));
					painter.setPen(Qt::NoPen);
					painter.drawPath(shape.path);
					painter.restore();
				}
				painter.setBrush(fill_color);
				painter.setPen(Qt::NoPen);
				painter.drawPath(shape.path);
				painter.restore();

				if (draw_platform_id) {
					const QString label = platform.id;
					painter.setPen(selected ? QColor(QStringLiteral("#111827")) : QColor(QStringLiteral("#334155")));
					const QSizeF text_size(metrics.horizontalAdvance(label), metrics.height());
					QRectF label_rect(center.x() + radius + 5.0,
						center.y() - text_size.height() * 0.50,
						text_size.width() + 8.0,
						text_size.height() + 2.0);
					painter.drawText(label_rect.adjusted(4.0, 0.0, 0.0, -2.0),
						Qt::AlignLeft | Qt::AlignVCenter, label);
				}
			}
			drawScaleBar(painter, projection, metrics, item_width, item_height);
			painter.end();

			root->overlay_node = new QSGSimpleTextureNode();
			QSGTexture* texture = window()->createTextureFromImage(overlay_image);
			root->overlay_node->setTexture(texture);
			root->overlay_node->setOwnsTexture(true);
			root->overlay_node->setRect(QRectF(0.0, 0.0, item_width, item_height));
			root->appendLayerAfter(root->overlay_node,
				root->selection_layer != nullptr ? root->selection_layer :
				(root->vector_layer != nullptr ? root->vector_layer : root->grid_layer));
		}
		root->overlay_revision = overlay_revision;
		root->overlay_device_ratio = device_ratio;
	}

	return root;
}

void UiInitMap::geometryChange(const QRectF& new_geometry, const QRectF& old_geometry)
{
	QQuickItem::geometryChange(new_geometry, old_geometry);
	if (new_geometry.size() != old_geometry.size()) {
		++geometry_revision;
		++vector_revision;
		++selection_revision;
		++overlay_revision;
		update();
	}
}

void UiInitMap::mousePressEvent(QMouseEvent* event)
{
	const QString platform_id = platformAt(event->position().x(), event->position().y());
	if (!platform_id.isEmpty()) {
		if (event->button() == Qt::RightButton) {
			emit platformRightClicked(platform_id);
		}
		else {
			emit platformClicked(platform_id);
		}
		event->accept();
		return;
	}

	if (event->button() != Qt::LeftButton) {
		event->ignore();
		return;
	}

	panning = true;
	last_pan_pos = event->position();
	setCursor(Qt::ClosedHandCursor);
	event->accept();
}

void UiInitMap::mouseMoveEvent(QMouseEvent* event)
{
	if (!panning) {
		event->ignore();
		return;
	}

	const QPointF pos = event->position();
	view_offset += pos - last_pan_pos;
	last_pan_pos = pos;
	invalidateView();
	event->accept();
}

void UiInitMap::mouseReleaseEvent(QMouseEvent* event)
{
	if (!panning) {
		event->ignore();
		return;
	}

	panning = false;
	unsetCursor();
	event->accept();
}

void UiInitMap::wheelEvent(QWheelEvent* event)
{
	const QPointF cursor_pos = event->position();
	const int delta = event->angleDelta().y();
	if (delta == 0) {
		event->ignore();
		return;
	}

	const qreal old_scale = view_scale;
	const qreal factor = std::pow(1.0015, static_cast<qreal>(delta));
	view_scale = std::clamp(view_scale * factor, static_cast<qreal>(0.45), static_cast<qreal>(8.0));
	if (std::abs(view_scale - old_scale) < 0.0001) {
		event->accept();
		return;
	}

	view_offset = cursor_pos - (cursor_pos - view_offset) * (view_scale / old_scale);
	invalidateView();
	event->accept();
}

void UiInitMap::rebuildBounds()
{
	double min_lon = 128.0;
	double max_lon = 137.0;
	double min_lat = 37.6;
	double max_lat = 40.2;

	if (!platforms.isEmpty()) {
		min_lon = max_lon = platforms.front().lon_deg;
		min_lat = max_lat = platforms.front().lat_deg;
	}
	for (int i = 1; i < platforms.size(); ++i) {
		min_lon = std::min(min_lon, platforms[i].lon_deg);
		max_lon = std::max(max_lon, platforms[i].lon_deg);
		min_lat = std::min(min_lat, platforms[i].lat_deg);
		max_lat = std::max(max_lat, platforms[i].lat_deg);
	}

	const double lon_pad = std::max(0.15, (max_lon - min_lon) * 0.08);
	const double lat_pad = std::max(0.15, (max_lat - min_lat) * 0.10);
	scene_bounds.min_lon = min_lon - lon_pad;
	scene_bounds.max_lon = max_lon + lon_pad;
	scene_bounds.min_lat = min_lat - lat_pad;
	scene_bounds.max_lat = max_lat + lat_pad;
}

bool UiInitMap::samePlatforms(const QVector<Platform>& next_platforms) const
{
	if (platforms.size() != next_platforms.size()) return false;

	auto same_double = [](double lhs, double rhs) {
		return std::abs(lhs - rhs) <= 0.000001;
	};

	for (int platform_index = 0; platform_index < platforms.size(); ++platform_index) {
		const Platform& lhs = platforms[platform_index];
		const Platform& rhs = next_platforms[platform_index];
		if (lhs.id != rhs.id ||
			lhs.commander != rhs.commander ||
			lhs.side != rhs.side ||
			lhs.icon != rhs.icon ||
			lhs.entity_kind != rhs.entity_kind ||
			!same_double(lhs.lon_deg, rhs.lon_deg) ||
			!same_double(lhs.lat_deg, rhs.lat_deg) ||
			!same_double(lhs.heading_deg, rhs.heading_deg) ||
			!same_double(lhs.max_sensor_range_m, rhs.max_sensor_range_m) ||
			!same_double(lhs.max_weapon_range_m, rhs.max_weapon_range_m) ||
			lhs.route_points.size() != rhs.route_points.size()) {
			return false;
		}

		for (int point_index = 0; point_index < lhs.route_points.size(); ++point_index) {
			if (!same_double(lhs.route_points[point_index].x(), rhs.route_points[point_index].x()) ||
				!same_double(lhs.route_points[point_index].y(), rhs.route_points[point_index].y())) {
				return false;
			}
		}
	}

	return true;
}

UiInitMap::MapProjection UiInitMap::currentProjection() const
{
	MapProjection projection;
	projection.center_lon = (scene_bounds.min_lon + scene_bounds.max_lon) * 0.5;
	projection.center_lat = (scene_bounds.min_lat + scene_bounds.max_lat) * 0.5;
	projection.meters_per_deg_lat = kMetersPerDegLat;
	projection.meters_per_deg_lon = std::max(10000.0,
		kMetersPerDegLat * std::cos(projection.center_lat * std::numbers::pi / 180.0));

	const qreal plot_width = std::max<qreal>(1.0, width() - kPlotPadding * 2.0);
	const qreal plot_height = std::max<qreal>(1.0, height() - kPlotPadding * 2.0);
	const double bounds_width_m = std::max(1.0,
		(scene_bounds.max_lon - scene_bounds.min_lon) * projection.meters_per_deg_lon);
	const double bounds_height_m = std::max(1.0,
		(scene_bounds.max_lat - scene_bounds.min_lat) * projection.meters_per_deg_lat);

	projection.scene_center_x = kPlotPadding + plot_width * 0.5;
	projection.scene_center_y = kPlotPadding + plot_height * 0.5;
	projection.scene_pixels_per_meter = std::min(plot_width / bounds_width_m, plot_height / bounds_height_m);
	return projection;
}

qreal UiInitMap::mapLon(double lon_deg) const
{
	const MapProjection projection = currentProjection();
	const qreal scene_x = projection.scene_center_x +
		(lon_deg - projection.center_lon) * projection.meters_per_deg_lon * projection.scene_pixels_per_meter;
	return scene_x * view_scale + view_offset.x();
}

qreal UiInitMap::mapLat(double lat_deg) const
{
	const MapProjection projection = currentProjection();
	const qreal scene_y = projection.scene_center_y -
		(lat_deg - projection.center_lat) * projection.meters_per_deg_lat * projection.scene_pixels_per_meter;
	return scene_y * view_scale + view_offset.y();
}

QPointF UiInitMap::mapPoint(double lon_deg, double lat_deg) const
{
	const MapProjection projection = currentProjection();
	const qreal scene_x = projection.scene_center_x +
		(lon_deg - projection.center_lon) * projection.meters_per_deg_lon * projection.scene_pixels_per_meter;
	const qreal scene_y = projection.scene_center_y -
		(lat_deg - projection.center_lat) * projection.meters_per_deg_lat * projection.scene_pixels_per_meter;
	return QPointF(scene_x * view_scale + view_offset.x(),
		scene_y * view_scale + view_offset.y());
}

qreal UiInitMap::rangePixels(const Platform& platform, double range_m) const
{
	Q_UNUSED(platform);
	if (range_m <= 0.0) return 0.0;
	return range_m * currentProjection().scene_pixels_per_meter * view_scale;
}

void UiInitMap::drawScaleBar(QPainter& painter, const MapProjection& projection,
	const QFontMetricsF& metrics, qreal item_width, qreal item_height) const
{
	const qreal pixels_per_meter = projection.scene_pixels_per_meter * view_scale;
	if (pixels_per_meter <= 0.0) return;

	const qreal target_width = std::clamp(item_width * 0.18, static_cast<qreal>(80.0), static_cast<qreal>(180.0));
	const qreal distance_m = nice_scale_distance_m(target_width / pixels_per_meter);
	if (distance_m <= 0.0) return;

	const qreal bar_width = distance_m * pixels_per_meter;
	if (bar_width < 24.0) return;

	const qreal left = std::max<qreal>(14.0, kPlotPadding * 0.5);
	const qreal bottom = item_height - std::max<qreal>(18.0, kPlotPadding * 0.55);
	const qreal tick_height = 7.0;
	const QString label = format_scale_distance(distance_m);
	const QSizeF text_size(metrics.horizontalAdvance(label), metrics.height());
	const QRectF background(left - 8.0, bottom - tick_height - text_size.height() - 8.0,
		std::max(bar_width, text_size.width()) + 16.0, tick_height + text_size.height() + 16.0);

	painter.save();
	Q_UNUSED(background);

	const QColor pen_color(QStringLiteral("#1e293b"));
	QPen pen(pen_color, 2.0);
	pen.setCapStyle(Qt::SquareCap);
	painter.setPen(pen);
	painter.drawLine(QPointF(left, bottom), QPointF(left + bar_width, bottom));
	painter.drawLine(QPointF(left, bottom), QPointF(left, bottom - tick_height));
	painter.drawLine(QPointF(left + bar_width, bottom), QPointF(left + bar_width, bottom - tick_height));

	painter.setPen(QColor(QStringLiteral("#334155")));
	painter.drawText(QRectF(left, bottom - tick_height - text_size.height() - 2.0,
		std::max(bar_width, text_size.width()), text_size.height()),
		Qt::AlignLeft | Qt::AlignVCenter, label);
	painter.restore();
}

const UiInitMap::IconShape& UiInitMap::iconShape(const QString& icon)
{
	const QString cache_key = normalize_icon_key(icon);
	if (const auto iter = icon_shape_cache.constFind(cache_key); iter != icon_shape_cache.constEnd()) {
		return iter.value();
	}

	IconShape shape;
	QVector<QPointF> vertices;
	QVector<QVector<int>> faces;
	const QString mesh_path = mesh_path_for_icon(icon);
	QFile file(mesh_path);
	if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		vertices.reserve(4096);
		faces.reserve(4096);
		while (!file.atEnd()) {
			const QString line = QString::fromUtf8(file.readLine()).trimmed();
			if (line.startsWith(QStringLiteral("v "))) {
				const QStringList parts = line.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
				if (parts.size() < 4) continue;

				bool ok_x = false;
				bool ok_z = false;
				const double x = parts[1].toDouble(&ok_x);
				const double z = parts[3].toDouble(&ok_z);
				if (ok_x && ok_z) {
					vertices.push_back(QPointF(x, z));
				}
			}
			else if (line.startsWith(QStringLiteral("f "))) {
				const QStringList parts = line.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
				if (parts.size() < 4) continue;

				QVector<int> face;
				face.reserve(parts.size() - 1);
				for (int i = 1; i < parts.size(); ++i) {
					const int index = parse_face_vertex_index(parts[i], vertices.size());
					if (index >= 0) {
						face.push_back(index);
					}
				}
				if (face.size() >= 3) {
					faces.push_back(face);
				}
			}
		}
	}

	if (vertices.size() >= 3) {
		qreal min_x = vertices.front().x();
		qreal max_x = vertices.front().x();
		qreal min_y = vertices.front().y();
		qreal max_y = vertices.front().y();
		for (const QPointF& point : vertices) {
			min_x = std::min(min_x, point.x());
			max_x = std::max(max_x, point.x());
			min_y = std::min(min_y, point.y());
			max_y = std::max(max_y, point.y());
		}

		const QPointF center((min_x + max_x) * 0.5, (min_y + max_y) * 0.5);
		const qreal scale = std::max<qreal>(0.0001, std::max(max_x - min_x, max_y - min_y) * 0.5);
		auto normalize = [&](const QPointF& point) {
			return QPointF((point.x() - center.x()) / scale, (point.y() - center.y()) / scale);
		};

		QVector<QPointF> normalized_hull;
		const QVector<QPointF> hull = convex_hull(vertices);
		normalized_hull.reserve(hull.size());
		for (const QPointF& point : hull) {
			normalized_hull.push_back(normalize(point));
		}
		const QPainterPath outline_path = closed_path_from_points(normalized_hull);

		QPainterPath path;
		path.setFillRule(Qt::WindingFill);
		if (!faces.isEmpty()) {
			std::sort(faces.begin(), faces.end(), [&](const QVector<int>& a, const QVector<int>& b) {
				return std::abs(polygon_area(vertices, a)) > std::abs(polygon_area(vertices, b));
			});

			const qsizetype face_count = std::min<qsizetype>(faces.size(), 1800);
			for (qsizetype face_index = 0; face_index < face_count; ++face_index) {
				const QVector<int>& face = faces[face_index];
				if (polygon_area(vertices, face) >= 0.0) {
					path.moveTo(normalize(vertices[face[0]]));
					for (int i = 1; i < face.size(); ++i) {
						path.lineTo(normalize(vertices[face[i]]));
					}
				}
				else {
					path.moveTo(normalize(vertices[face.last()]));
					for (int i = face.size() - 2; i >= 0; --i) {
						path.lineTo(normalize(vertices[face[i]]));
					}
				}
				path.closeSubpath();
			}
		}
		else {
			path = outline_path;
		}

		if (!path.isEmpty()) {
			shape.path = path;
			shape.outline_path = outline_path.isEmpty() ? path : outline_path;
			shape.valid = true;
		}
	}

	if (!shape.valid) {
		shape.path = normalize_icon_key(icon).contains(QStringLiteral("mis")) ||
			normalize_icon_key(icon).contains(QStringLiteral("missile")) ?
			fallback_missile_path() : fallback_aircraft_path();
		shape.outline_path = shape.path;
		shape.valid = true;
	}

	icon_shape_cache.insert(cache_key, shape);
	return icon_shape_cache[cache_key];
}
