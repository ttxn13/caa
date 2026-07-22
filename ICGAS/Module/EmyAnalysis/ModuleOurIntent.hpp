#pragma once
#include "pch.hpp"

class ModuleOurIntent : public QObject
{
	Q_OBJECT

public:
	static ModuleOurIntent* GetInstance();
	static void DstInstance();

private:
	explicit ModuleOurIntent(QObject* parent = nullptr);
	~ModuleOurIntent() override;

private:
	static QMutex mutex;
	static ModuleOurIntent* instance;
};
