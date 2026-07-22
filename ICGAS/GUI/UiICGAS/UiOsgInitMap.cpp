#include "pch.hpp"
#include "UiOsgInitMap.hpp"

#include "../../OsgEarth/src/AfsMapViewer.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLWindow>
#include <QScreen>
#include <QSurfaceFormat>
#include <QWheelEvent>

#include <osg/Camera>
#include <osg/Depth>
#include <osg/DisplaySettings>
#include <osg/Matrixd>
#include <osg/State>
#include <osg/Vec3d>
#include <osg/Vec4d>
#include <osgDB/Registry>
#include <osgEarth/GLUtils>
#include <osgGA/EventQueue>
#include <osgViewer/GraphicsWindow>
#include <osgViewer/Viewer>

#include <algorithm>
#include <cmath>
#include <cstdlib>

void ensure_osgearth_runtime_initialized();

namespace {
constexpr qreal kPlotPadding = 36.0;
constexpr qreal kIconBaseSize = 24.0;
constexpr double kMetersPerDegLat = 111320.0;
constexpr double kWgs84A = 6378137.0;
constexpr double kWgs84B = 6356752.314245;
constexpr int kInteractiveFrameMs = 16;
constexpr int kSettleFrameMs = 33;
#ifdef _DEBUG
constexpr int kInitialQuickRefineMs = 12000;
constexpr int kViewRefineMs = 8000;
#else
constexpr int kInitialQuickRefineMs = 900;
constexpr int kViewRefineMs = 1800;
#endif
const osg::Vec4 kSceneClearColor(0.04f, 0.055f, 0.075f, 1.0f);

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

bool is_formation_entity_kind(const QString& entity_kind)
{
	const QString normalized = entity_kind.trimmed().toLower();
	return normalized == QStringLiteral("format") || normalized == QStringLiteral("formation");
}

std::string to_native_path(const QString& path)
{
	return QDir::toNativeSeparators(path).toStdString();
}

QString repo_root_path()
{
	const QDir app_dir(QCoreApplication::applicationDirPath());
	const QString cwd = QDir::currentPath();
	const QStringList candidates = {
		QDir(cwd).absoluteFilePath(QStringLiteral(".")),
		QDir(cwd).absoluteFilePath(QStringLiteral("..")),
		QDir(app_dir).absoluteFilePath(QStringLiteral("..")),
		QDir(app_dir).absoluteFilePath(QStringLiteral("../.."))
	};

	for (const QString& candidate : candidates) {
		const QDir dir(QDir::cleanPath(candidate));
		if (QFileInfo::exists(dir.filePath(QStringLiteral("ICGAS/ICGAS.vcxproj")))) {
			return dir.absolutePath();
		}
	}

	return QDir(cwd).absolutePath();
}

QString mesh_dir_path()
{
	const QDir app_dir(QCoreApplication::applicationDirPath());
	const QString root = repo_root_path();
	const QStringList candidates = {
		app_dir.filePath(QStringLiteral("App/mesh")),
		app_dir.filePath(QStringLiteral("../App/mesh")),
		app_dir.filePath(QStringLiteral("mesh")),
		QDir(root).filePath(QStringLiteral("ICGAS/App/mesh")),
		QDir::current().filePath(QStringLiteral("ICGAS/App/mesh")),
		QDir::current().filePath(QStringLiteral("App/mesh"))
	};
	for (const QString& candidate : candidates) {
		if (QDir(candidate).exists()) return QDir(candidate).absolutePath();
	}
	return QDir(root).filePath(QStringLiteral("ICGAS/App/mesh"));
}

QString first_existing_mesh(const QDir& mesh_dir, const QStringList& names)
{
	for (const QString& name : names) {
		const QFileInfo info(mesh_dir.filePath(name));
		if (info.exists()) return info.absoluteFilePath();
	}
	return QString();
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

QString source_mesh_alias_for_key(const QDir& mesh_dir, const QString& key)
{
	if (key.isEmpty() ||
		key == QStringLiteral("core.fixedwing") ||
		key == QStringLiteral("fixedwing") ||
		key == QStringLiteral("core.aircraft")) {
		return first_existing_mesh(mesh_dir, {
			QStringLiteral("FixedWing.F-18E.obj"),
			QStringLiteral("FixedWing.F-15.obj"),
			QStringLiteral("FixedWing.F-35.obj"),
			QStringLiteral("FixedWing.F-16.obj"),
			QStringLiteral("Core.FixedWing.obj")
		});
	}
	if (key == QStringLiteral("core.watercraft") ||
		key == QStringLiteral("watercraft") ||
		key == QStringLiteral("core.ship")) {
		return first_existing_mesh(mesh_dir, {
			QStringLiteral("Watercraft.Type 052D.obj"),
			QStringLiteral("Watercraft.CVN-59.obj"),
			QStringLiteral("Watercraft.CVN-68.obj"),
			QStringLiteral("Core.Watercraft.obj")
		});
	}
	if (key == QStringLiteral("core.aircraftcarrier") ||
		key == QStringLiteral("aircraftcarrier") ||
		key == QStringLiteral("carrier")) {
		return first_existing_mesh(mesh_dir, {
			QStringLiteral("Watercraft.CVN-59.obj"),
			QStringLiteral("Watercraft.CVN-68.obj"),
			QStringLiteral("Core.AircraftCarrier.obj")
		});
	}
	if (key == QStringLiteral("core.vehicle") ||
		key == QStringLiteral("vehicle")) {
		return first_existing_mesh(mesh_dir, {
			QStringLiteral("Vehicle.Tank.M1.obj"),
			QStringLiteral("Vehicle.Car.Humvee.obj"),
			QStringLiteral("Vehicle.Truck.Ural.obj"),
			QStringLiteral("Core.Vehicle.obj")
		});
	}
	if (key == QStringLiteral("core.rotorcraft") ||
		key == QStringLiteral("rotorcraft")) {
		return first_existing_mesh(mesh_dir, {
			QStringLiteral("Rotorcraft.UH-60.obj"),
			QStringLiteral("Rotorcraft.AH-64.obj"),
			QStringLiteral("Core.Rotorcraft.obj")
		});
	}
	return QString();
}

QString mesh_path_for_icon(const QString& icon)
{
	const QString key = normalize_icon_key(icon);
	const QDir mesh_dir(mesh_dir_path());
	const QString aliased_source_mesh = source_mesh_alias_for_key(mesh_dir, key);
	if (!aliased_source_mesh.isEmpty()) return aliased_source_mesh;

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
		const QString source_mesh = first_existing_mesh(mesh_dir, {
			QStringLiteral("Watercraft.Type 052D.obj"),
			QStringLiteral("Watercraft.CVN-59.obj"),
			QStringLiteral("Watercraft.CVN-68.obj"),
			QStringLiteral("Core.Watercraft.obj")
		});
		if (!source_mesh.isEmpty()) return source_mesh;
	}
	if (key.contains(QStringLiteral("sam")) || key.contains(QStringLiteral("truck")) || key.contains(QStringLiteral("tank"))) {
		const QString source_mesh = first_existing_mesh(mesh_dir, {
			QStringLiteral("Vehicle.Tank.M1.obj"),
			QStringLiteral("Vehicle.Car.Humvee.obj"),
			QStringLiteral("Vehicle.Truck.Ural.obj"),
			QStringLiteral("Core.Vehicle.obj")
		});
		if (!source_mesh.isEmpty()) return source_mesh;
	}
	const QString source_mesh = first_existing_mesh(mesh_dir, {
		QStringLiteral("FixedWing.F-18E.obj"),
		QStringLiteral("FixedWing.F-15.obj"),
		QStringLiteral("FixedWing.F-35.obj"),
		QStringLiteral("FixedWing.F-16.obj"),
		QStringLiteral("Core.FixedWing.obj")
	});
	return source_mesh.isEmpty() ? mesh_dir.filePath(QStringLiteral("Core.FixedWing.obj")) : source_mesh;
}

qreal platform_hit_radius(qreal icon_scale)
{
	return std::max<qreal>(2.0, kIconBaseSize * 0.5 * icon_scale);
}

QString initial_map_resource_root()
{
	const QString root = repo_root_path();
	const QDir app_dir(QCoreApplication::applicationDirPath());
	const QStringList candidates = {
		app_dir.filePath(QStringLiteral("OsgEarth/map")),
		QDir(root).filePath(QStringLiteral("ICGAS/OsgEarth/map")),
		QDir::current().filePath(QStringLiteral("ICGAS/OsgEarth/map")),
		app_dir.filePath(QStringLiteral("maps")),
		QDir(root).filePath(QStringLiteral("afsim-2.9.0-win64/resources/maps")),
		QDir::current().filePath(QStringLiteral("afsim-2.9.0-win64/resources/maps")),
		app_dir.filePath(QStringLiteral("../../../afsim-2.9.0-win64/resources/maps")),
		app_dir.filePath(QStringLiteral("../../../../afsim-2.9.0-win64/resources/maps"))
	};
	for (const QString& candidate : candidates) {
		const QString clean = QDir::cleanPath(candidate);
		if (QDir(clean).exists()) return clean;
	}
	return QDir(root).filePath(QStringLiteral("ICGAS/OsgEarth/map"));
}

int osg_button(Qt::MouseButton button)
{
	switch (button) {
	case Qt::LeftButton: return 1;
	case Qt::MiddleButton: return 2;
	case Qt::RightButton: return 3;
	default: return 0;
	}
}

bool intersect_wgs84(const osg::Vec3d& origin, const osg::Vec3d& direction, osg::Vec3d& world)
{
	const double a2 = kWgs84A * kWgs84A;
	const double b2 = kWgs84B * kWgs84B;
	const double qa = (direction.x() * direction.x() + direction.y() * direction.y()) / a2 +
		(direction.z() * direction.z()) / b2;
	const double qb = 2.0 * ((origin.x() * direction.x() + origin.y() * direction.y()) / a2 +
		(origin.z() * direction.z()) / b2);
	const double qc = (origin.x() * origin.x() + origin.y() * origin.y()) / a2 +
		(origin.z() * origin.z()) / b2 - 1.0;
	const double discriminant = qb * qb - 4.0 * qa * qc;
	if (qa <= 0.0 || discriminant < 0.0) return false;

	const double root = std::sqrt(discriminant);
	const double t0 = (-qb - root) / (2.0 * qa);
	const double t1 = (-qb + root) / (2.0 * qa);
	const double t = t0 > 0.0 ? t0 : t1;
	if (t <= 0.0) return false;

	world = origin + direction * t;
	return true;
}

osg::Vec3d lla_to_ecef(double lon_deg, double lat_deg, double alt_m)
{
	const double lon = lon_deg * std::numbers::pi / 180.0;
	const double lat = lat_deg * std::numbers::pi / 180.0;
	const double sin_lat = std::sin(lat);
	const double cos_lat = std::cos(lat);
	const double e2 = 6.69437999014e-3;
	const double n = kWgs84A / std::sqrt(1.0 - e2 * sin_lat * sin_lat);
	return osg::Vec3d(
		(n + alt_m) * cos_lat * std::cos(lon),
		(n + alt_m) * cos_lat * std::sin(lon),
		(n * (1.0 - e2) + alt_m) * sin_lat);
}

}

struct UiOsgInitMap::SharedState
{
	mutable QMutex mutex;
	osg::Matrixd view_matrix;
	osg::Matrixd projection_matrix;
	osg::Matrixd view_projection_matrix;
	QSize viewport_size;
	bool camera_valid = false;
	qreal last_geo_x = 0.0;
	qreal last_geo_y = 0.0;
	QString last_platform_id;
};

class UiOsgInitMapWindow final : public QOpenGLWindow
{
public:
	explicit UiOsgInitMapWindow(UiOsgInitMap* owner)
		: QOpenGLWindow(QOpenGLWindow::NoPartialUpdate),
		owner_(owner)
	{
		QSurfaceFormat opaque_format = QSurfaceFormat::defaultFormat();
		opaque_format.setRenderableType(QSurfaceFormat::OpenGL);
		opaque_format.setVersion(3, 3);
		opaque_format.setProfile(QSurfaceFormat::CompatibilityProfile);
		opaque_format.setOption(QSurfaceFormat::DeprecatedFunctions);
		opaque_format.setAlphaBufferSize(0);
		opaque_format.setDepthBufferSize(24);
		opaque_format.setStencilBufferSize(8);
		opaque_format.setSamples(0);
		opaque_format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
		setFormat(opaque_format);
		setOpacity(1.0);
		setTitle(QStringLiteral("ICGAS Initial Situation Map"));
		connect(&render_timer_, &QTimer::timeout, this, [this]() {
			if (!interactive_rendering_ && render_clock_.elapsed() > render_until_ms_) {
				render_timer_.stop();
				return;
			}
			update();
		});
		render_timer_.setTimerType(Qt::PreciseTimer);
		render_clock_.start();
		connect(this, &QWindow::screenChanged, this, [this](QScreen*) {
			refreshAfterSurfaceChange();
		});
	}

	~UiOsgInitMapWindow() override = default;

	void setScene(const QVector<UiOsgInitMap::Platform>& platforms,
		const QString& selected_platform_id,
		quint64 scene_revision)
	{
		platforms_ = platforms;
		selected_platform_id_ = selected_platform_id;
		scene_revision_ = scene_revision;
		const bool scene_applied = applySceneIfNeeded();
		requestRenderBurst(scene_applied ? kViewRefineMs : 120);
		update();
	}

protected:
	void initializeGL() override
	{
		ensure_osgearth_runtime_initialized();
		startup_placeholder_submitted_ = false;
		high_dpi_scale_ = devicePixelRatio();
		const int scaled_width = std::max(1, static_cast<int>(width() * high_dpi_scale_));
		const int scaled_height = std::max(1, static_cast<int>(height() * high_dpi_scale_));
		clearOpaqueBackBuffer();

		osgDB::Registry::instance()->getDataFilePathList().push_back(to_native_path(mesh_dir_path()));

		graphics_window_ = new osgViewer::GraphicsWindowEmbedded(0, 0, scaled_width, scaled_height);
		if (osg::State* state = graphics_window_->getState()) {
			state->resetVertexAttributeAlias(false);
			state->setUseModelViewAndProjectionUniforms(true);
			state->setUseVertexAttributeAliasing(true);
		}

		viewer_ = new osgViewer::Viewer;
		viewer_->setThreadingModel(osgViewer::Viewer::SingleThreaded);

		osg::ref_ptr<osg::Camera> camera = viewer_->getCamera();
		camera->setGraphicsContext(graphics_window_.get());
		camera->setViewport(0, 0, scaled_width, scaled_height);
		camera->setClearColor(kSceneClearColor);
		camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		camera->setClearStencil(0);
		camera->setProjectionMatrixAsPerspective(35.0,
			double(scaled_width) / double(std::max(1, scaled_height)),
			1.0, 1.0e9);
		camera->getOrCreateStateSet()->setAttributeAndModes(new osg::Depth);

		requestMapLoad();
		requestRenderBurst(kInitialQuickRefineMs);
	}

	void paintGL() override
	{
		clearOpaqueBackBuffer();
		if (!startup_placeholder_submitted_) {
			startup_placeholder_submitted_ = true;
			forceOpaqueBackBufferAlpha();
			update();
			return;
		}

		if (!viewer_.valid()) {
			forceOpaqueBackBufferAlpha();
			return;
		}

		loadPendingMaps();
		if (!map_viewer_) {
			forceOpaqueBackBufferAlpha();
			return;
		}

		applySceneIfNeeded();
		flushPendingMouseMotion();
		map_viewer_->update();
		viewer_->frame();
		forceOpaqueBackBufferAlpha();
		publishCamera();
		first_frame_submitted_ = true;
	}

	void resizeGL(int w, int h) override
	{
		high_dpi_scale_ = devicePixelRatio();
		const int scaled_width = std::max(1, static_cast<int>(w * high_dpi_scale_));
		const int scaled_height = std::max(1, static_cast<int>(h * high_dpi_scale_));

		if (graphics_window_.valid()) {
			graphics_window_->resized(0, 0, scaled_width, scaled_height);
			graphics_window_->getEventQueue()->windowResize(0, 0, scaled_width, scaled_height);
		}
		if (viewer_.valid() && viewer_->getCamera()) {
			viewer_->getCamera()->setViewport(0, 0, scaled_width, scaled_height);
			viewer_->getCamera()->setProjectionMatrixAsPerspective(35.0,
				double(scaled_width) / double(std::max(1, scaled_height)),
				1.0, 1.0e9);
		}
		clearOpaqueBackBuffer();
		requestRenderBurst(kViewRefineMs);
	}

	bool event(QEvent* event) override
	{
		const QEvent::Type type = event->type();
		const bool handled = QOpenGLWindow::event(event);
		switch (type) {
		case QEvent::Expose:
		case QEvent::Show:
		case QEvent::Resize:
		case QEvent::WindowStateChange:
		case QEvent::DevicePixelRatioChange:
			refreshAfterSurfaceChange();
			break;
		default:
			break;
		}
		return handled;
	}

	void mousePressEvent(QMouseEvent* event) override
	{
		setInteractiveRendering(true);
		requestActivate();
		resetMouseMotionCoalescing(event->buttons(), event->position());
		const QPointF pos = scaledPos(event->position());

		if (map_viewer_ && (event->button() == Qt::LeftButton || event->button() == Qt::RightButton)) {
			const QPointF local_pos = event->position();
			const QString id = owner_ ? owner_->cameraPlatformAt(local_pos.x(), local_pos.y()) : QString();
			if (!id.isEmpty()) {
				map_viewer_->selectPlatform(id.toStdString());
				if (event->button() == Qt::RightButton) {
					if (owner_) owner_->emitWindowPlatformRightClicked(id);
				}
				else {
					if (owner_) owner_->emitWindowPlatformClicked(id);
				}
				event->accept();
				requestRenderBurst(480);
				update();
				return;
			}
		}

		if (graphics_window_.valid()) {
			graphics_window_->getEventQueue()->mouseButtonPress(pos.x(), pos.y(), osg_button(event->button()));
			forwarded_mouse_buttons_ |= event->button();
		}

		event->accept();
		requestRenderBurst(480);
		update();
	}

	void mouseReleaseEvent(QMouseEvent* event) override
	{
		const QPointF pos = scaledPos(event->position());
		if (graphics_window_.valid() && forwarded_mouse_buttons_.testFlag(event->button())) {
			queueMouseMotion(event->position(), forwarded_mouse_buttons_);
			flushPendingMouseMotion();
			graphics_window_->getEventQueue()->mouseButtonRelease(pos.x(), pos.y(), osg_button(event->button()));
			forwarded_mouse_buttons_ &= ~event->button();
		}
		if (forwarded_mouse_buttons_ == Qt::NoButton) {
			clearPendingMouseMotion();
		}
		event->accept();
		setInteractiveRendering(forwarded_mouse_buttons_ != Qt::NoButton);
		update();
	}

	void mouseMoveEvent(QMouseEvent* event) override
	{
		const bool forwarding_drag = (forwarded_mouse_buttons_ & event->buttons()) != Qt::NoButton;
		if (graphics_window_.valid() && forwarding_drag) {
			queueMouseMotion(event->position(), event->buttons());
		}
		if (map_viewer_ && event->buttons() != Qt::NoButton) {
			map_viewer_->interruptFollow();
		}
		event->accept();
		setInteractiveRendering(forwarding_drag);
		if (forwarding_drag) {
			requestRenderBurst(360);
		}
		update();
	}

	void mouseDoubleClickEvent(QMouseEvent* event) override
	{
		event->accept();
		update();
	}

	void wheelEvent(QWheelEvent* event) override
	{
		const QPoint angle_delta = event->angleDelta();
		const QPoint pixel_delta = event->pixelDelta();
		const double wheel_steps = angle_delta.y() != 0
			? double(angle_delta.y()) / 120.0
			: double(pixel_delta.y()) / 50.0;
		if (map_viewer_) {
			map_viewer_->interruptFollow();
			if (map_viewer_->zoomByWheelSteps(wheel_steps)) {
				requestRenderBurst(kViewRefineMs);
			}
		}
		event->accept();
		setInteractiveRendering(true);
		QTimer::singleShot(80, this, [this]() {
			setInteractiveRendering(false);
		});
		update();
	}

	void keyPressEvent(QKeyEvent* event) override
	{
		if (map_viewer_ && event->key() == Qt::Key_Home) {
			if (event->modifiers() & Qt::AltModifier) {
				if (!map_viewer_->selectedPlatformId().empty()) {
					map_viewer_->centerOnPlatform(map_viewer_->selectedPlatformId());
				}
			}
			else {
				map_viewer_->home();
			}
			event->accept();
			setInteractiveRendering(true);
			requestRenderBurst(kViewRefineMs);
			QTimer::singleShot(480, this, [this]() {
				setInteractiveRendering(false);
			});
			update();
			return;
		}

		if (graphics_window_.valid()) {
			graphics_window_->getEventQueue()->keyPress(static_cast<osgGA::GUIEventAdapter::KeySymbol>(event->key()));
		}
		event->accept();
		requestRenderBurst(360);
		update();
	}

	void keyReleaseEvent(QKeyEvent* event) override
	{
		if (graphics_window_.valid()) {
			graphics_window_->getEventQueue()->keyRelease(static_cast<osgGA::GUIEventAdapter::KeySymbol>(event->key()));
		}
		event->accept();
		requestRenderBurst(240);
		update();
	}

private:
	AfsPlatformState toAfsPlatformState(const UiOsgInitMap::Platform& platform) const
	{
		AfsPlatformState state;
		state.id = platform.id.toStdString();
		state.name = platform.id.toStdString();
		state.side = platform.side.toStdString();
		state.commanderId = platform.commander.toStdString();
		state.entityKind = platform.entity_kind.toStdString();
		state.icon = to_native_path(mesh_path_for_icon(platform.icon));
		state.lonDeg = platform.lon_deg;
		state.latDeg = platform.lat_deg;
		state.altM = platform.altitude_m;
		state.headingDeg = platform.heading_deg;
		state.pathAngleDeg = platform.path_angle_deg;
		state.speedMps = platform.speed_mps;
		state.formationRangeM = platform.formation_range_m;
		state.visible = true;
		state.selected = platform.id == selected_platform_id_;
		state.showLabel = owner_ && (is_formation_entity_kind(platform.entity_kind)
			? owner_->draw_formation_range
			: owner_->draw_platform_id);
		const bool include_preset_route = owner_ ? (owner_->draw_route && owner_->draw_preset_route) : true;
		const bool include_actual_route = owner_ ? (owner_->draw_route && owner_->draw_actual_route) : true;
		if (include_preset_route) {
			state.route.reserve(platform.route_points.size());
			for (const UiOsgInitMap::RoutePoint& point : platform.route_points) {
				AfsRoutePoint route_point;
				route_point.lonDeg = point.lon_deg;
				route_point.latDeg = point.lat_deg;
				route_point.altM = point.altitude_m;
				route_point.speedMps = point.speed_mps;
				state.route.push_back(route_point);
			}
		}
		if (include_actual_route) {
			state.trail.reserve(platform.trail_points.size());
			for (const UiOsgInitMap::RoutePoint& point : platform.trail_points) {
				AfsRoutePoint trail_point;
				trail_point.lonDeg = point.lon_deg;
				trail_point.latDeg = point.lat_deg;
				trail_point.altM = point.altitude_m;
				trail_point.speedMps = point.speed_mps;
				state.trail.push_back(trail_point);
			}
		}
		return state;
	}

	bool applySceneIfNeeded()
	{
		if (!map_viewer_ || applied_scene_revision_ == scene_revision_) return false;

		std::vector<AfsPlatformState> states;
		states.reserve(platforms_.size());
		for (const UiOsgInitMap::Platform& platform : platforms_) {
			states.push_back(toAfsPlatformState(platform));
		}
		if (owner_) {
			map_viewer_->setUseSourcePlatformModels(owner_->use_source_models);
			map_viewer_->setShowRoutes(owner_->draw_route &&
				(owner_->draw_preset_route || owner_->draw_actual_route));
			map_viewer_->setShowCommandLinks(owner_->draw_command_link);
			map_viewer_->setShowFormationRanges(owner_->draw_formation_range);
		}
		map_viewer_->upsertPlatforms(states);
		if (!selected_platform_id_.isEmpty()) {
			map_viewer_->selectPlatform(selected_platform_id_.toStdString());
		}
		else {
			map_viewer_->selectPlatform(std::string());
		}
		if (!had_platforms_ && !states.empty()) {
			map_viewer_->centerOnPlatforms(states, 0.0);
		}
		had_platforms_ = !states.empty();
		applied_scene_revision_ = scene_revision_;
		return true;
	}

	void publishCamera()
	{
		if (!viewer_.valid() || !viewer_->getCamera()) return;
		if (!owner_ || !owner_->shared_state) return;

		QMutexLocker locker(&owner_->shared_state->mutex);
		owner_->shared_state->view_matrix = viewer_->getCamera()->getViewMatrix();
		owner_->shared_state->projection_matrix = viewer_->getCamera()->getProjectionMatrix();
		owner_->shared_state->view_projection_matrix =
			owner_->shared_state->view_matrix * owner_->shared_state->projection_matrix;
		owner_->shared_state->viewport_size = QSize(
			std::max(1, static_cast<int>(width() * high_dpi_scale_)),
			std::max(1, static_cast<int>(height() * high_dpi_scale_)));
		owner_->shared_state->camera_valid = true;
	}

	QPointF scaledPos(const QPointF& pos) const
	{
		return QPointF(pos.x() * high_dpi_scale_, pos.y() * high_dpi_scale_);
	}

	void requestMapLoad()
	{
		if (map_loaded_) {
			return;
		}

		map_load_requested_ = true;
		update();
	}

	void loadPendingMaps()
	{
		if (map_load_requested_) {
			map_load_requested_ = false;
			loadMapIfNeeded();
		}
	}

	void loadMapIfNeeded()
	{
		if (map_loaded_ || !viewer_.valid()) {
			return;
		}

		clearOpaqueBackBuffer();

		map_viewer_ = std::make_unique<AfsMapViewer>();
		map_viewer_->initialize(viewer_.get());
		map_viewer_->preferences().setResourceRoot(initial_map_resource_root().toStdString());
		map_viewer_->setDefaultBoundaryOverlayEnabled(true);
		map_viewer_->setUseSourcePlatformModels(true);
		const AfsLayerLoadResult result = map_viewer_->loadMapByName(map_viewer_->preferences().defaultMapName());
		if (!result.ok) {
			const AfsLayerLoadResult fallback_result = map_viewer_->loadMapByName("Bald Earth");
			if (!fallback_result.ok) {
				map_viewer_->setFallbackGlobeEnabled(true);
			}
		}

		map_loaded_ = true;
		applySceneIfNeeded();
		requestRenderBurst(kInitialQuickRefineMs);
		update();
	}

	void clearOpaqueBackBuffer()
	{
		QOpenGLFunctions* functions = context() ? context()->functions() : nullptr;
		if (!functions) {
			return;
		}

		const int scaled_width = std::max(1, static_cast<int>(width() * high_dpi_scale_));
		const int scaled_height = std::max(1, static_cast<int>(height() * high_dpi_scale_));
		functions->glViewport(0, 0, scaled_width, scaled_height);
		functions->glDisable(GL_SCISSOR_TEST);
		functions->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		functions->glClearColor(kSceneClearColor.r(), kSceneClearColor.g(), kSceneClearColor.b(), kSceneClearColor.a());
		functions->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

	void forceOpaqueBackBufferAlpha()
	{
		QOpenGLFunctions* functions = context() ? context()->functions() : nullptr;
		if (!functions) {
			return;
		}

		functions->glDisable(GL_SCISSOR_TEST);
		functions->glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
		functions->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		functions->glClear(GL_COLOR_BUFFER_BIT);
		functions->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	}

	void refreshAfterSurfaceChange()
	{
		high_dpi_scale_ = devicePixelRatio();
		requestRenderBurst(first_frame_submitted_ ? kViewRefineMs : kInitialQuickRefineMs);
		update();
	}

	void resetMouseMotionCoalescing(Qt::MouseButtons buttons, const QPointF& pos)
	{
		pending_mouse_motion_ = false;
		last_mouse_motion_valid_ = true;
		last_mouse_motion_buttons_ = buttons;
		last_mouse_motion_pos_ = scaledPos(pos);
	}

	void queueMouseMotion(const QPointF& pos, Qt::MouseButtons buttons)
	{
		const QPointF scaled_pos = scaledPos(pos);
		if (last_mouse_motion_valid_
			&& last_mouse_motion_buttons_ == buttons
			&& qFuzzyCompare(last_mouse_motion_pos_.x() + 1.0, scaled_pos.x() + 1.0)
			&& qFuzzyCompare(last_mouse_motion_pos_.y() + 1.0, scaled_pos.y() + 1.0)) {
			return;
		}

		pending_mouse_motion_ = true;
		pending_mouse_motion_pos_ = scaled_pos;
		last_mouse_motion_valid_ = true;
		last_mouse_motion_buttons_ = buttons;
		last_mouse_motion_pos_ = scaled_pos;
	}

	void flushPendingMouseMotion()
	{
		if (!pending_mouse_motion_ || !graphics_window_.valid()) {
			return;
		}

		graphics_window_->getEventQueue()->mouseMotion(pending_mouse_motion_pos_.x(), pending_mouse_motion_pos_.y());
		pending_mouse_motion_ = false;
	}

	void clearPendingMouseMotion()
	{
		pending_mouse_motion_ = false;
		last_mouse_motion_valid_ = false;
		last_mouse_motion_buttons_ = Qt::NoButton;
	}

	void requestRenderBurst(int duration_ms)
	{
		if (!render_clock_.isValid()) {
			render_clock_.start();
		}
		render_until_ms_ = std::max<qint64>(render_until_ms_, render_clock_.elapsed() + std::max(1, duration_ms));
		render_timer_.setInterval(interactive_rendering_ ? kInteractiveFrameMs : kSettleFrameMs);
		if (!render_timer_.isActive()) {
			render_timer_.start();
		}
		update();
	}

	void setInteractiveRendering(bool interactive)
	{
		if (interactive_rendering_ == interactive) {
			return;
		}
		interactive_rendering_ = interactive;
		render_timer_.setInterval(interactive ? kInteractiveFrameMs : kSettleFrameMs);
		if (interactive) {
			requestRenderBurst(1000);
		}
		else {
			requestRenderBurst(480);
		}
	}

	UiOsgInitMap* owner_ = nullptr;
	QVector<UiOsgInitMap::Platform> platforms_;
	QString selected_platform_id_;
	quint64 scene_revision_ = 0;
	quint64 applied_scene_revision_ = 0;
	bool had_platforms_ = false;
	bool map_load_requested_ = false;
	bool map_loaded_ = false;
	bool startup_placeholder_submitted_ = false;
	bool first_frame_submitted_ = false;
	bool interactive_rendering_ = false;
	Qt::MouseButtons forwarded_mouse_buttons_;
	bool pending_mouse_motion_ = false;
	bool last_mouse_motion_valid_ = false;
	Qt::MouseButtons last_mouse_motion_buttons_ = Qt::NoButton;
	QPointF pending_mouse_motion_pos_;
	QPointF last_mouse_motion_pos_;
	osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> graphics_window_;
	osg::ref_ptr<osgViewer::Viewer> viewer_;
	std::unique_ptr<AfsMapViewer> map_viewer_;
	QTimer render_timer_;
	QElapsedTimer render_clock_;
	qint64 render_until_ms_ = 0;
	double high_dpi_scale_ = 1.0;
};

UiOsgInitMap::UiOsgInitMap(QQuickItem* parent)
	: QQuickItem(parent),
	shared_state(std::make_shared<SharedState>())
{
	setFlag(ItemHasContents, false);
}

UiOsgInitMap::~UiOsgInitMap() = default;

QWindow* UiOsgInitMap::mapWindow() const
{
	return ensureMapWindow();
}

QVariantList UiOsgInitMap::platformRows() const
{
	QVariantList rows;
	rows.reserve(platforms.size());
	for (const Platform& platform : platforms) {
		QVariantMap row;
		row[QStringLiteral("platform_id")] = platform.id;
		row[QStringLiteral("commander")] = platform.commander;
		row[QStringLiteral("side")] = platform.side;
		row[QStringLiteral("icon")] = platform.icon;
		row[QStringLiteral("entity_kind")] = platform.entity_kind;
		row[QStringLiteral("lon_deg")] = platform.lon_deg;
		row[QStringLiteral("lat_deg")] = platform.lat_deg;
		row[QStringLiteral("altitude_m")] = platform.altitude_m;
		row[QStringLiteral("speed_mps")] = platform.speed_mps;
		row[QStringLiteral("heading_deg")] = platform.heading_deg;
		row[QStringLiteral("path_angle_deg")] = platform.path_angle_deg;
		row[QStringLiteral("max_sensor_range_m")] = platform.max_sensor_range_m;
		row[QStringLiteral("max_weapon_range_m")] = platform.max_weapon_range_m;
		row[QStringLiteral("formation_range_m")] = platform.formation_range_m;
		QVariantList route_points;
		route_points.reserve(platform.route_points.size());
		for (const RoutePoint& point : platform.route_points) {
			QVariantMap route_point;
			route_point[QStringLiteral("lon_deg")] = point.lon_deg;
			route_point[QStringLiteral("lat_deg")] = point.lat_deg;
			route_point[QStringLiteral("altitude_m")] = point.altitude_m;
			route_point[QStringLiteral("speed_mps")] = point.speed_mps;
			route_points.push_back(route_point);
		}
		row[QStringLiteral("route_points")] = route_points;
		QVariantList trail_points;
		trail_points.reserve(platform.trail_points.size());
		for (const RoutePoint& point : platform.trail_points) {
			QVariantMap trail_point;
			trail_point[QStringLiteral("lon_deg")] = point.lon_deg;
			trail_point[QStringLiteral("lat_deg")] = point.lat_deg;
			trail_point[QStringLiteral("altitude_m")] = point.altitude_m;
			trail_point[QStringLiteral("speed_mps")] = point.speed_mps;
			trail_points.push_back(trail_point);
		}
		row[QStringLiteral("trail_points")] = trail_points;
		rows.push_back(row);
	}
	return rows;
}

void UiOsgInitMap::setPlatformRows(const QVariantList& rows)
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
		platform.altitude_m = map_double(row, QStringLiteral("altitude_m"));
		platform.speed_mps = map_double(row, QStringLiteral("speed_mps"));
		platform.heading_deg = map_double(row, QStringLiteral("heading_deg"));
		platform.path_angle_deg = map_double(row, QStringLiteral("path_angle_deg"));
		platform.max_sensor_range_m = map_double(row, QStringLiteral("max_sensor_range_m"));
		platform.max_weapon_range_m = map_double(row, QStringLiteral("max_weapon_range_m"));
		platform.formation_range_m = map_double(row, QStringLiteral("formation_range_m"));

		const QVariantList route_points = row.value(QStringLiteral("route_points")).toList();
		platform.route_points.reserve(route_points.size());
		for (const QVariant& route_value : route_points) {
			const QVariantMap route_point = route_value.toMap();
			RoutePoint point;
			point.lon_deg = map_double(route_point, QStringLiteral("lon_deg"));
			point.lat_deg = map_double(route_point, QStringLiteral("lat_deg"));
			point.altitude_m = map_double(route_point, QStringLiteral("altitude_m"), platform.altitude_m);
			point.speed_mps = map_double(route_point, QStringLiteral("speed_mps"), platform.speed_mps);
			platform.route_points.push_back(point);
		}

		const QVariantList trail_points = row.value(QStringLiteral("trail_points")).toList();
		platform.trail_points.reserve(trail_points.size());
		for (const QVariant& trail_value : trail_points) {
			const QVariantMap trail_point = trail_value.toMap();
			RoutePoint point;
			point.lon_deg = map_double(trail_point, QStringLiteral("lon_deg"));
			point.lat_deg = map_double(trail_point, QStringLiteral("lat_deg"));
			point.altitude_m = map_double(trail_point, QStringLiteral("altitude_m"), platform.altitude_m);
			point.speed_mps = map_double(trail_point, QStringLiteral("speed_mps"), platform.speed_mps);
			platform.trail_points.push_back(point);
		}

		if (!platform.id.isEmpty()) {
			next_platform_index.insert(platform.id, next_platforms.size());
		}
		next_platforms.push_back(platform);
	}

	if (samePlatforms(next_platforms)) return;

	platforms = std::move(next_platforms);
	platform_index = std::move(next_platform_index);
	rebuildBounds();
	++scene_revision;
	syncWindowScene();
	update();
	emit platformRowsChanged();
}

QString UiOsgInitMap::selectedPlatformId() const
{
	return selected_platform_id;
}

void UiOsgInitMap::setSelectedPlatformId(const QString& platform_id)
{
	if (selected_platform_id == platform_id) return;
	selected_platform_id = platform_id;
	++scene_revision;
	syncWindowScene();
	update();
	emit selectedPlatformIdChanged();
}

bool UiOsgInitMap::showPlatformId() const
{
	return draw_platform_id;
}

void UiOsgInitMap::setShowPlatformId(bool visible)
{
	if (draw_platform_id == visible) return;
	draw_platform_id = visible;
	++scene_revision;
	syncWindowScene();
	update();
	emit showPlatformIdChanged();
}

bool UiOsgInitMap::showCommandLink() const
{
	return draw_command_link;
}

void UiOsgInitMap::setShowCommandLink(bool visible)
{
	if (draw_command_link == visible) return;
	draw_command_link = visible;
	++scene_revision;
	syncWindowScene();
	update();
	emit showCommandLinkChanged();
}

bool UiOsgInitMap::showSensorRange() const
{
	return draw_sensor_range;
}

void UiOsgInitMap::setShowSensorRange(bool visible)
{
	if (draw_sensor_range == visible) return;
	draw_sensor_range = visible;
	++scene_revision;
	update();
	emit showSensorRangeChanged();
}

bool UiOsgInitMap::showWeaponRange() const
{
	return draw_weapon_range;
}

void UiOsgInitMap::setShowWeaponRange(bool visible)
{
	if (draw_weapon_range == visible) return;
	draw_weapon_range = visible;
	++scene_revision;
	update();
	emit showWeaponRangeChanged();
}

bool UiOsgInitMap::showFormationRange() const
{
	return draw_formation_range;
}

void UiOsgInitMap::setShowFormationRange(bool visible)
{
	if (draw_formation_range == visible) return;
	draw_formation_range = visible;
	++scene_revision;
	syncWindowScene();
	update();
	emit showFormationRangeChanged();
}

bool UiOsgInitMap::showRoute() const
{
	return draw_route;
}

void UiOsgInitMap::setShowRoute(bool visible)
{
	if (draw_route == visible) return;
	draw_route = visible;
	++scene_revision;
	syncWindowScene();
	update();
	emit showRouteChanged();
}

bool UiOsgInitMap::showPresetRoute() const
{
	return draw_preset_route;
}

void UiOsgInitMap::setShowPresetRoute(bool visible)
{
	if (draw_preset_route == visible) return;
	draw_preset_route = visible;
	++scene_revision;
	syncWindowScene();
	update();
	emit showPresetRouteChanged();
}

bool UiOsgInitMap::showActualRoute() const
{
	return draw_actual_route;
}

void UiOsgInitMap::setShowActualRoute(bool visible)
{
	if (draw_actual_route == visible) return;
	draw_actual_route = visible;
	++scene_revision;
	syncWindowScene();
	update();
	emit showActualRouteChanged();
}

QString UiOsgInitMap::labelFontFamily() const
{
	return label_font_family;
}

void UiOsgInitMap::setLabelFontFamily(const QString& family)
{
	if (label_font_family == family) return;
	label_font_family = family;
	++scene_revision;
	update();
	emit labelFontFamilyChanged();
}

int UiOsgInitMap::labelPixelSize() const
{
	return label_pixel_size;
}

void UiOsgInitMap::setLabelPixelSize(int pixel_size)
{
	const int normalized_size = std::max(1, pixel_size);
	if (label_pixel_size == normalized_size) return;
	label_pixel_size = normalized_size;
	++scene_revision;
	update();
	emit labelPixelSizeChanged();
}

qreal UiOsgInitMap::iconScale() const
{
	return icon_scale;
}

void UiOsgInitMap::setIconScale(qreal scale)
{
	const qreal normalized_scale = std::clamp(scale, qreal(0.35), qreal(2.0));
	if (qFuzzyCompare(icon_scale, normalized_scale)) return;
	icon_scale = normalized_scale;
	++scene_revision;
	update();
	emit iconScaleChanged();
}

bool UiOsgInitMap::useSourceModels() const
{
	return use_source_models;
}

void UiOsgInitMap::setUseSourceModels(bool enabled)
{
	if (use_source_models == enabled) return;
	use_source_models = enabled;
	++scene_revision;
	syncWindowScene();
	update();
	emit useSourceModelsChanged();
}

QString UiOsgInitMap::platformAt(qreal x, qreal y) const
{
	const QString camera_platform = cameraPlatformAt(x, y);
	if (!camera_platform.isEmpty()) return camera_platform;

	QString best_id;
	const qreal hit_radius = platform_hit_radius(icon_scale);
	qreal best_distance_squared = hit_radius * hit_radius;
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

QVariantMap UiOsgInitMap::geoAt(qreal x, qreal y) const
{
	double lon = 0.0;
	double lat = 0.0;
	if (cameraGeoAt(x, y, lon, lat)) {
		QVariantMap result;
		result[QStringLiteral("lon_deg")] = lon;
		result[QStringLiteral("lat_deg")] = lat;
		return result;
	}
	return geoAtFallback(x, y);
}

double UiOsgInitMap::lonAt(qreal x) const
{
	double lon = 0.0;
	double lat = 0.0;
	qreal y = height() * 0.5;
	if (shared_state) {
		QMutexLocker locker(&shared_state->mutex);
		y = shared_state->last_geo_y > 0.0 ? shared_state->last_geo_y : y;
	}
	if (cameraGeoAt(x, y, lon, lat)) return lon;
	return geoAtFallback(x, height() * 0.5).value(QStringLiteral("lon_deg")).toDouble();
}

double UiOsgInitMap::latAt(qreal y) const
{
	double lon = 0.0;
	double lat = 0.0;
	qreal x = width() * 0.5;
	if (shared_state) {
		QMutexLocker locker(&shared_state->mutex);
		x = shared_state->last_geo_x > 0.0 ? shared_state->last_geo_x : x;
	}
	if (cameraGeoAt(x, y, lon, lat)) return lat;
	return geoAtFallback(width() * 0.5, y).value(QStringLiteral("lat_deg")).toDouble();
}

void UiOsgInitMap::requestRedraw()
{
	++scene_revision;
	syncWindowScene();
	update();
}

void UiOsgInitMap::geometryChange(const QRectF& new_geometry, const QRectF& old_geometry)
{
	QQuickItem::geometryChange(new_geometry, old_geometry);
	if (new_geometry.size() != old_geometry.size()) {
		++scene_revision;
		syncWindowScene();
		update();
	}
}

void UiOsgInitMap::itemChange(ItemChange change, const ItemChangeData& value)
{
	QQuickItem::itemChange(change, value);
	if (change == ItemSceneChange) {
		syncMapWindowScreen();
	}
}

void UiOsgInitMap::rebuildBounds()
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

bool UiOsgInitMap::samePlatforms(const QVector<Platform>& next_platforms) const
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
			!same_double(lhs.altitude_m, rhs.altitude_m) ||
			!same_double(lhs.speed_mps, rhs.speed_mps) ||
			!same_double(lhs.heading_deg, rhs.heading_deg) ||
			!same_double(lhs.path_angle_deg, rhs.path_angle_deg) ||
			!same_double(lhs.max_sensor_range_m, rhs.max_sensor_range_m) ||
			!same_double(lhs.max_weapon_range_m, rhs.max_weapon_range_m) ||
			!same_double(lhs.formation_range_m, rhs.formation_range_m) ||
			lhs.route_points.size() != rhs.route_points.size() ||
			lhs.trail_points.size() != rhs.trail_points.size()) {
			return false;
		}

		for (int point_index = 0; point_index < lhs.route_points.size(); ++point_index) {
			if (!same_double(lhs.route_points[point_index].lon_deg, rhs.route_points[point_index].lon_deg) ||
				!same_double(lhs.route_points[point_index].lat_deg, rhs.route_points[point_index].lat_deg) ||
				!same_double(lhs.route_points[point_index].altitude_m, rhs.route_points[point_index].altitude_m) ||
				!same_double(lhs.route_points[point_index].speed_mps, rhs.route_points[point_index].speed_mps)) {
				return false;
			}
		}

		for (int point_index = 0; point_index < lhs.trail_points.size(); ++point_index) {
			if (!same_double(lhs.trail_points[point_index].lon_deg, rhs.trail_points[point_index].lon_deg) ||
				!same_double(lhs.trail_points[point_index].lat_deg, rhs.trail_points[point_index].lat_deg) ||
				!same_double(lhs.trail_points[point_index].altitude_m, rhs.trail_points[point_index].altitude_m) ||
				!same_double(lhs.trail_points[point_index].speed_mps, rhs.trail_points[point_index].speed_mps)) {
				return false;
			}
		}
	}

	return true;
}

UiOsgInitMap::MapProjection UiOsgInitMap::currentProjection() const
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

QPointF UiOsgInitMap::mapPoint(double lon_deg, double lat_deg) const
{
	const MapProjection projection = currentProjection();
	const qreal scene_x = projection.scene_center_x +
		(lon_deg - projection.center_lon) * projection.meters_per_deg_lon * projection.scene_pixels_per_meter;
	const qreal scene_y = projection.scene_center_y -
		(lat_deg - projection.center_lat) * projection.meters_per_deg_lat * projection.scene_pixels_per_meter;
	return QPointF(scene_x * view_scale + view_offset.x(),
		scene_y * view_scale + view_offset.y());
}

QVariantMap UiOsgInitMap::geoAtFallback(qreal x, qreal y) const
{
	const MapProjection projection = currentProjection();
	const qreal scene_x = (x - view_offset.x()) / std::max<qreal>(0.01, view_scale);
	const qreal scene_y = (y - view_offset.y()) / std::max<qreal>(0.01, view_scale);
	const double lon = projection.center_lon +
		(scene_x - projection.scene_center_x) /
		std::max<qreal>(0.000001, projection.scene_pixels_per_meter) /
		projection.meters_per_deg_lon;
	const double lat = projection.center_lat -
		(scene_y - projection.scene_center_y) /
		std::max<qreal>(0.000001, projection.scene_pixels_per_meter) /
		projection.meters_per_deg_lat;

	QVariantMap result;
	result[QStringLiteral("lon_deg")] = std::clamp(lon, scene_bounds.min_lon, scene_bounds.max_lon);
	result[QStringLiteral("lat_deg")] = std::clamp(lat, scene_bounds.min_lat, scene_bounds.max_lat);
	return result;
}

bool UiOsgInitMap::cameraGeoAt(qreal x, qreal y, double& lon_deg, double& lat_deg) const
{
	if (!shared_state) return false;

	QMutexLocker locker(&shared_state->mutex);
	if (!shared_state->camera_valid || shared_state->viewport_size.width() <= 0 || shared_state->viewport_size.height() <= 0) {
		return false;
	}

	const double vx = std::clamp(x / std::max<qreal>(1.0, width()), 0.0, 1.0);
	const double vy = std::clamp(y / std::max<qreal>(1.0, height()), 0.0, 1.0);
	const double ndc_x = vx * 2.0 - 1.0;
	const double ndc_y = 1.0 - vy * 2.0;
	const osg::Matrixd inverse_vp = osg::Matrixd::inverse(shared_state->view_projection_matrix);
	const osg::Vec3d near_point = osg::Vec3d(ndc_x, ndc_y, -1.0) * inverse_vp;
	const osg::Vec3d far_point = osg::Vec3d(ndc_x, ndc_y, 1.0) * inverse_vp;
	osg::Vec3d direction = far_point - near_point;
	if (direction.normalize() <= 0.0) return false;

	osg::Vec3d world;
	if (!intersect_wgs84(near_point, direction, world)) return false;

	const double lon = std::atan2(world.y(), world.x()) * 180.0 / std::numbers::pi;
	const double p = std::hypot(world.x(), world.y());
	const double lat = std::atan2(world.z(), p * (1.0 - 6.69437999014e-3)) * 180.0 / std::numbers::pi;
	lon_deg = lon;
	lat_deg = lat;
	shared_state->last_geo_x = x;
	shared_state->last_geo_y = y;
	return true;
}

QString UiOsgInitMap::cameraPlatformAt(qreal x, qreal y) const
{
	osg::Matrixd view_projection;
	bool camera_valid = false;
	if (shared_state) {
		QMutexLocker locker(&shared_state->mutex);
		camera_valid = shared_state->camera_valid;
		view_projection = shared_state->view_projection_matrix;
	}

	QString best_id;
	const qreal hit_radius = platform_hit_radius(icon_scale);
	qreal best_distance_squared = hit_radius * hit_radius;
	for (const Platform& platform : platforms) {
		QPointF point;
		if (camera_valid) {
			const osg::Vec3d world = lla_to_ecef(platform.lon_deg, platform.lat_deg, platform.altitude_m);
			const osg::Vec3d ndc = world * view_projection;
			if (ndc.z() < -1.0 || ndc.z() > 1.0) continue;
			point = QPointF((ndc.x() * 0.5 + 0.5) * width(), (0.5 - ndc.y() * 0.5) * height());
		}
		else {
			point = mapPoint(platform.lon_deg, platform.lat_deg);
		}
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

UiOsgInitMapWindow* UiOsgInitMap::ensureMapWindow() const
{
	if (!map_window) {
		map_window = std::make_unique<UiOsgInitMapWindow>(const_cast<UiOsgInitMap*>(this));
		syncMapWindowScreen();
		map_window->setScene(platforms, selected_platform_id, scene_revision);
	}
	return map_window.get();
}

void UiOsgInitMap::syncMapWindowScreen() const
{
	if (!map_window) {
		return;
	}

	QScreen* target_screen = nullptr;
	if (QQuickWindow* item_window = window()) {
		target_screen = item_window->screen();
	}
	const QList<QScreen*> screens = QGuiApplication::screens();
	if (target_screen == nullptr || !screens.contains(target_screen)) {
		target_screen = QGuiApplication::primaryScreen();
	}
	if (target_screen != nullptr && map_window->screen() != target_screen && !map_window->isVisible()) {
		map_window->setScreen(target_screen);
	}
}

void UiOsgInitMap::syncWindowScene()
{
	if (map_window) {
		map_window->setScene(platforms, selected_platform_id, scene_revision);
	}
}

void UiOsgInitMap::emitWindowPlatformClicked(const QString& platform_id)
{
	if (!platform_id.isEmpty()) {
		emit platformClicked(platform_id);
	}
}

void UiOsgInitMap::emitWindowPlatformRightClicked(const QString& platform_id)
{
	if (!platform_id.isEmpty()) {
		emit platformRightClicked(platform_id);
	}
}
