#pragma once
#include "pch.hpp"

class ModuleEmyIntent : public QObject
{
	Q_OBJECT
	friend class ModuleDatabase;

public:
	static ModuleEmyIntent* GetInstance();
	static void DstInstance();

private:
	explicit ModuleEmyIntent(QObject* parent = nullptr);
	~ModuleEmyIntent() override;

private:
	static QMutex mutex;
	static ModuleEmyIntent* instance;
};
