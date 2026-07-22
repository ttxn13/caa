#pragma once
#include "pch.hpp"

class ModuleBvrSolver : public QObject
{
	Q_OBJECT

public:
	static ModuleBvrSolver* GetInstance();
	static void DstInstance();

private:
	explicit ModuleBvrSolver(QObject* parent = nullptr);
	~ModuleBvrSolver() override;

private:
	static QMutex mutex;
	static ModuleBvrSolver* instance;
};
