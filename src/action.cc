#include "action.hh"
#include "run.hh"

MyActionInitialization::MyActionInitialization()
{}

MyActionInitialization::~MyActionInitialization()
{}

void MyActionInitialization::Build() const
{
    MyPrimaryGenerator *generator = new MyPrimaryGenerator();
    SetUserAction(generator);

    MyRunAction *runAction = new MyRunAction();
    SetUserAction(runAction);

    // Construct stepping first with nullptr, then give it to event
    MySteppingAction *steppingAction = new MySteppingAction(nullptr);
    MyEventAction *eventAction = new MyEventAction(steppingAction);
    steppingAction->SetEventAction(eventAction);  // set it after both exist

    SetUserAction(eventAction);
    SetUserAction(steppingAction);

    MyTrackingAction *trackingAction = new MyTrackingAction(steppingAction);
    SetUserAction(trackingAction);
}