#include "ControlSystem.hpp"

ControlSystem::ControlSystem(double dt)
    : g(2.0),
      q1("quat1"),
      signalChecker(-0.2, 0.2),
      motor("motor1"),
      motorVoltageSetpoint(0.0),
      timedomain("Main time domain", dt, true)
{
    // Name all blocks
    q1.setName("q1");
    g.setName("g");
    signalChecker.setName("signalChecker");
    motorVoltageSetpoint.setName("motorVoltageSetpoint");
    motor.setName("motor");

    // Name all signals
    q1.getOut().getSignal().setName("alpha/2");
    g.getOut().getSignal().setName("alpha");
    motorVoltageSetpoint.getOut().getSignal().setName("motorVoltageSetpoint");

    // Connect signals
    g.getIn().connect(q1.getOut());
    motor.getIn().connect(motorVoltageSetpoint.getOut());

    // Add blocks to timedomain
    timedomain.addBlock(q1);
    timedomain.addBlock(g);
    timedomain.addBlock(motorVoltageSetpoint);
    timedomain.addBlock(motor);

    // Add timedomain to executor
    eeros::Executor::instance().add(timedomain);
}