#include "pch.hpp"
#include "ModuleBvrSolver.hpp"

QMutex ModuleBvrSolver::mutex;
ModuleBvrSolver* ModuleBvrSolver::instance = nullptr;

ModuleBvrSolver* ModuleBvrSolver::GetInstance()
{
	if (instance == nullptr) {
		mutex.lock();
		if (instance == nullptr) {
			instance = new ModuleBvrSolver();
		}
		mutex.unlock();
	}
	return instance;
}

void ModuleBvrSolver::DstInstance()
{
	delete instance;
	instance = nullptr;
}

ModuleBvrSolver::ModuleBvrSolver(QObject* parent)
	: QObject(parent)
{
}

ModuleBvrSolver::~ModuleBvrSolver()
{
}
