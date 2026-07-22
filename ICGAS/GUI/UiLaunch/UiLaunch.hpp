#pragma once
#include "pch.hpp"
#include "../../Core/IncCore.hpp"

#include "../UiICGAS/UiICGAS.hpp"

class InfoExpert;

class UiLaunch : public QObject
{
	Q_OBJECT

public:
	static UiLaunch*	GetInstance();
	static void			DstInstance();
	explicit UiLaunch(QObject* parent = nullptr);
	~UiLaunch();

	void show();
	void setWindowIcon(const QIcon& icon);

private slots:
	void on_login_clicked(const QString& user, const QString& pass);
	void on_start_clicked();
	void on_close_clicked();

private:
	QObject* root_object() const;
	void center_on_primary_screen(QQuickWindow* window);
	void init_ui_state();
	bool check_login(const QString& user, const QString& pass) const;
	void close_launch_window();

private:
	QQmlApplicationEngine	engine;
	QIcon					window_icon;
	static QMutex			mutex;
	static UiLaunch*		instance;
	InfoExpert*				expert = nullptr;
	bool					start_enabled = false;
};
