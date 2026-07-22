#pragma once
#include "pch.hpp"

class ModuleEmyCOG : public QObject
{
	Q_OBJECT

public:
	static ModuleEmyCOG* GetInstance();
	static void DstInstance();

private:
	explicit ModuleEmyCOG(QObject* parent = nullptr);
	~ModuleEmyCOG() override;

private:
	static QMutex mutex;
	static ModuleEmyCOG* instance;
};
