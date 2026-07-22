#pragma once
#include "pch.hpp"

class ModuleEmyLOO : public QObject
{
	Q_OBJECT

public:
	static ModuleEmyLOO* GetInstance();
	static void DstInstance();

private:
	explicit ModuleEmyLOO(QObject* parent = nullptr);
	~ModuleEmyLOO() override;

private:
	static QMutex mutex;
	static ModuleEmyLOO* instance;
};
