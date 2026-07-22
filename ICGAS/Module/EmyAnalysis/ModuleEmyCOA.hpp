#pragma once
#include "pch.hpp"

class ModuleEmyCOA : public QObject
{
	Q_OBJECT

public:
	static ModuleEmyCOA* GetInstance();
	static void DstInstance();

private:
	explicit ModuleEmyCOA(QObject* parent = nullptr);
	~ModuleEmyCOA() override;

private:
	static QMutex mutex;
	static ModuleEmyCOA* instance;
};
