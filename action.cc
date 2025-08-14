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

    //MyEventAction *eventAction = new MyEventAction(runAction);
	//SetUserAction(eventAction);
	
	MySteppingAction *steppingAction = new MySteppingAction(); //eventAction);
	SetUserAction(steppingAction);
	
	MyTrackingAction *trackingAction = new MyTrackingAction(steppingAction);
	SetUserAction(trackingAction);

}