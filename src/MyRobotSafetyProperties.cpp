#include "MyRobotSafetyProperties.hpp"

MyRobotSafetyProperties::MyRobotSafetyProperties(ControlSystem &cs, double dt)
    : cs(cs),
    
      // Initialize ALL Levels
      slSystemOff("System is offline"),
      slShuttingDown("Shutting down"),
      slBraking("Braking"),
      slStartingUp("Starting up"),
      slEmergency("Emergency"),
      slEmergencyBraking("Emergency braking"),
      slSystemOn("System is online"),
      slMotorPowerOn("Motors powered on"),
      slSystemMoving("System is moving"),
 
      // Initialize ALL Events
      doSystemOn("Startup the system"),
      doSystemOff("Shutdown the system"),
      motorsHalted("Motors halted"),
      abort("Abort"),
      emergency("Emergency"),
      resetEmergency("Reset emergency"),
      powerOn("Power on"),
      powerOff("Power off"),
      startingMoving("Start moving"),
      stopMoving("Stop moving"),
      systemStarted("System started")
{
    eeros::hal::HAL &hal = eeros::hal::HAL::instance();

    // Declare and add critical outputs
    // ... = hal.getLogicOutput("...");
        LEDred = hal.getLogicOutput("onBoardLEDred");
        LEDgreen = hal.getLogicOutput("onBoardLEDgreen");

    // criticalOutputs = { ... };
    criticalOutputs = { LEDred, LEDgreen };
    // Declare and add critical inputs
    // ... = eeros::hal::HAL::instance().getLogicInput("...", ...);

    ButtonPause = eeros::hal::HAL::instance().getLogicInput("onBoardButtonPause");
    ButtonMode = eeros::hal::HAL::instance().getLogicInput("onBoardButtonMode");  

    // criticalInputs = { ... };
    criticalInputs = { ButtonPause, ButtonMode };

    // Add all safety levels to the safety system
    addLevel(slSystemOff);
    addLevel(slShuttingDown);
    addLevel(slBraking);
    addLevel(slStartingUp);
    addLevel(slEmergency);
    addLevel(slEmergencyBraking);
    addLevel(slSystemOn);
    addLevel(slMotorPowerOn);
    addLevel(slSystemMoving);

    // Add events to individual safety levels
    slSystemOff.addEvent(doSystemOn, slStartingUp, kPublicEvent);
    slShuttingDown.addEvent(doSystemOff, slSystemOff, kPrivateEvent);
    slBraking.addEvent(motorsHalted, slShuttingDown, kPrivateEvent);
    slStartingUp.addEvent(systemStarted, slSystemOn, kPrivateEvent);
    slEmergency.addEvent(resetEmergency, slSystemOn, kPrivateEvent);
    slEmergencyBraking.addEvent(motorsHalted, slEmergency, kPrivateEvent);
    slSystemOn.addEvent(powerOn, slMotorPowerOn, kPublicEvent);
    slMotorPowerOn.addEvent(startingMoving, slSystemMoving, kPublicEvent);
    slMotorPowerOn.addEvent(powerOff, slSystemOn, kPublicEvent);
    slSystemMoving.addEvent(stopMoving, slMotorPowerOn, kPublicEvent);
    slSystemMoving.addEvent(abort, slBraking, kPublicEvent);
    slSystemMoving.addEvent(emergency, slEmergencyBraking, kPublicEvent);

    addEventToAllLevelsBetween(slSystemOn, slMotorPowerOn, emergency, slEmergency, kPublicEvent);
    addEventToAllLevelsBetween(slEmergency, slMotorPowerOn, abort, slShuttingDown, kPublicEvent);
    
    // Add events to multiple safety levels
    // addEventToAllLevelsBetween(lowerLevel, upperLevel, event, targetLevel, kPublicEvent/kPrivateEvent);

    // Define input actions for all levels
    // level.setInputActions({ ... });
    slSystemOff.setInputActions({           ignore(ButtonPause),                    ignore(ButtonMode) });
    slShuttingDown.setInputActions({        ignore(ButtonPause),                    ignore(ButtonMode) });
    slBraking.setInputActions({             ignore(ButtonPause),                    ignore(ButtonMode) });
    slStartingUp.setInputActions({          ignore(ButtonPause),                    ignore(ButtonMode) });
    slEmergency.setInputActions({           ignore(ButtonPause),                    check(ButtonMode, false, resetEmergency) });
    slEmergencyBraking.setInputActions({    ignore(ButtonPause),                    ignore(ButtonMode) });
    slSystemOn.setInputActions({            check(ButtonPause, false, emergency),   ignore(ButtonMode) });
    slMotorPowerOn.setInputActions({        check(ButtonPause, false, emergency),   ignore(ButtonMode) });
    slSystemMoving.setInputActions({        check(ButtonPause, false, emergency),   ignore(ButtonMode) });

    // Define output actions for all levels
    // level.setOutputActions({ ... });
    slSystemOff.setOutputActions({           set(LEDgreen, false),   set(LEDred, false) });
    slShuttingDown.setOutputActions({        set(LEDgreen, false),   set(LEDred, true) });
    slBraking.setOutputActions({             set(LEDgreen, false),   set(LEDred, true) });
    slStartingUp.setOutputActions({          set(LEDgreen, true),    set(LEDred, false) });
    slEmergency.setOutputActions({           set(LEDgreen, true),    set(LEDred, true) });
    slEmergencyBraking.setOutputActions({    set(LEDgreen, true),    set(LEDred, true) });
    slSystemOn.setOutputActions({            set(LEDgreen, true),    set(LEDred, false) });
    slMotorPowerOn.setOutputActions({        set(LEDgreen, true),    set(LEDred, false) });
    slSystemMoving.setOutputActions({        set(LEDgreen, true),    set(LEDred, false) });

    // Define and add level actions
    slSystemOff.setLevelAction([&](SafetyContext *privateContext) {
        cs.timedomain.stop();
        eeros::Executor::stop();
    });

    slShuttingDown.setLevelAction([&](SafetyContext *privateContext) {
        cs.timedomain.stop();
        privateContext->triggerEvent(doSystemOff);
    });

    slBraking.setLevelAction([&](SafetyContext *privateContext) {
        // Check if motors are standing sill
        privateContext->triggerEvent(motorsHalted);
    });

    
    slStartingUp.setLevelAction([&](SafetyContext *privateContext) {
        cs.timedomain.start();
        privateContext->triggerEvent(systemStarted);
    });

    slEmergency.setLevelAction([&](SafetyContext *privateContext) {
        
    });

    slEmergencyBraking.setLevelAction([&](SafetyContext *privateContext) {
        // Check if motors are standing still
        privateContext->triggerEvent(motorsHalted);
    });

    slSystemOn.setLevelAction([&, dt](SafetyContext *privateContext) {
        if (slSystemOn.getNofActivations()*dt >= 1)   // wait 1 sec
        {
            privateContext->triggerEvent(powerOn);
        }
    });

    slMotorPowerOn.setLevelAction([&, dt](SafetyContext *privateContext) {
        if (slMotorPowerOn.getNofActivations()*dt >= 5)   // wait 5 sec
        {
            privateContext->triggerEvent(startingMoving);
        }
    });

    slSystemMoving.setLevelAction([&, dt](SafetyContext *privateContext) {
        if (slSystemMoving.getNofActivations()*dt >= 5)   // wait 5 sec
        {
            privateContext->triggerEvent(stopMoving);
        }
    });
    // Define entry level
    setEntryLevel(slSystemOff);

    // Define exit function
    exitFunction = ([&](SafetyContext *privateContext) {
        privateContext->triggerEvent(doSystemOff);
    });
}
