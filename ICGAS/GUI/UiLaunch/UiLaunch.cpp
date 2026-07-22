#include "pch.hpp"
#include "UiLaunch.hpp"

QMutex UiLaunch::mutex;
UiLaunch* UiLaunch::instance = NULL;

UiLaunch* UiLaunch::GetInstance()
{
	if (instance == NULL) {
		mutex.lock();
		instance = new UiLaunch();
		mutex.unlock();
	}
	return instance;
}

void UiLaunch::DstInstance()
{
	delete instance;
	instance = NULL;
}

UiLaunch::UiLaunch(QObject* parent)
	: QObject(parent)
{
	this->expert = InfoExpert::GetInstance();
}

UiLaunch::~UiLaunch()
{
}

void UiLaunch::show()
{
	if (engine.rootObjects().isEmpty()) {
		engine.load(QUrl(QStringLiteral("qrc:/UiICGAS/UiLaunch.qml")));
		if (engine.rootObjects().isEmpty()) {
			qDebug() << "Error: 无法加载启动页QML";
			return;
		}

		QObject* root = root_object();
		connect(root, SIGNAL(loginClicked(QString,QString)), this, SLOT(on_login_clicked(QString,QString)));
		connect(root, SIGNAL(startClicked()), this, SLOT(on_start_clicked()));
		connect(root, SIGNAL(closeClicked()), this, SLOT(on_close_clicked()));
		init_ui_state();

		if (QQuickWindow* window = qobject_cast<QQuickWindow*>(root)) {
			window->setIcon(window_icon);
			window->show();
			QTimer::singleShot(0, window, [this, window]() {
				center_on_primary_screen(window);
				window->raise();
			});
		}
		else {
			root->setProperty("visible", true);
		}
		return;
	}

	if (QQuickWindow* window = qobject_cast<QQuickWindow*>(root_object())) {
		window->show();
		QTimer::singleShot(0, window, [this, window]() {
			center_on_primary_screen(window);
			window->raise();
		});
	}
	else if (QObject* root = root_object()) {
		root->setProperty("visible", true);
	}
}

void UiLaunch::setWindowIcon(const QIcon& icon)
{
	window_icon = icon;
	if (QQuickWindow* window = qobject_cast<QQuickWindow*>(root_object())) {
		window->setIcon(window_icon);
	}
}

QObject* UiLaunch::root_object() const
{
	return engine.rootObjects().isEmpty() ? nullptr : engine.rootObjects().first();
}

void UiLaunch::center_on_primary_screen(QQuickWindow* window)
{
	if (window == nullptr) {
		return;
	}

	QScreen* screen = window->screen();
	const QList<QScreen*> screens = QGuiApplication::screens();
	if (screen == nullptr || !screens.contains(screen)) {
		screen = QGuiApplication::primaryScreen();
	}
	if (screen == nullptr) {
		return;
	}

	const QRect geometry = screen->availableGeometry();
	if (geometry.width() <= 0 || geometry.height() <= 0) {
		return;
	}

	if (window->screen() != screen) {
		window->setScreen(screen);
	}

	QSize target_size(qMax(1, window->width()), qMax(1, window->height()));
	if (target_size.width() > geometry.width() || target_size.height() > geometry.height()) {
		target_size.scale(geometry.size(), Qt::KeepAspectRatio);
		window->resize(qMax(1, target_size.width()), qMax(1, target_size.height()));
	}

	const int x = geometry.x() + qMax(0, (geometry.width() - window->width()) / 2);
	const int y = geometry.y() + qMax(0, (geometry.height() - window->height()) / 2);
	window->setPosition(x, y);
}

void UiLaunch::init_ui_state()
{
	QObject* root = root_object();
	if (root == nullptr) {
		return;
	}

	QString user;
	QString pass;
	if (!expert->login_info.empty()) {
		user = expert->login_info.begin()->first;
	}

	start_enabled = false;
	if (expert->admin_mode && !expert->login_info.empty()) {
		pass = expert->login_info.begin()->second;
		start_enabled = true;
	}

	root->setProperty("userName", user);
	root->setProperty("password", pass);
	root->setProperty("messageText", QStringLiteral("请输入用户名及密码！"));
	root->setProperty("startEnabled", start_enabled);
}

bool UiLaunch::check_login(const QString& user, const QString& pass) const
{
	for (const auto& iter : expert->login_info) {
		if (user == iter.first && pass == iter.second) {
			return true;
		}
	}
	return false;
}

void UiLaunch::on_login_clicked(const QString& user, const QString& pass)
{
	QObject* root = root_object();
	if (root == nullptr) {
		return;
	}

	if (check_login(user, pass)) {
		start_enabled = true;
		root->setProperty("startEnabled", true);
		root->setProperty("messageText", QStringLiteral("验证通过！"));
		return;
	}

	if (!expert->admin_mode) {
		start_enabled = false;
		root->setProperty("startEnabled", false);
	}
	root->setProperty("messageText", QStringLiteral("用户名或密码错误！"));
}

void UiLaunch::on_start_clicked()
{
	if (!start_enabled) {
		return;
	}

	UiICGAS::GetInstance()->setWindowIcon(window_icon);
	UiICGAS::GetInstance()->show();
	close_launch_window();
}

void UiLaunch::on_close_clicked()
{
	close_launch_window();
	QCoreApplication::quit();
}

void UiLaunch::close_launch_window()
{
	if (QQuickWindow* window = qobject_cast<QQuickWindow*>(root_object())) {
		window->close();
	}
	else if (QObject* root = root_object()) {
		QMetaObject::invokeMethod(root, "close");
	}
}
