#ifndef RUN_HH
#define RUN_HH

#include "G4UserRunAction.hh"
#include "G4Run.hh"
#include "G4AnalysisManager.hh"
#include <chrono>

class MyRunAction : public G4UserRunAction
{
public:
    MyRunAction();
    ~MyRunAction();
    virtual void BeginOfRunAction(const G4Run*);
    virtual void EndOfRunAction(const G4Run*);
    std::chrono::high_resolution_clock::time_point GetStartTime() const
    { return startTime; }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> startTime;

};

#endif