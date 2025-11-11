# Networked Control System Simulation

This project simulates a distributed networked control system with two nodes that communicate to regulate and monitor a dynamic system in real-time. The simulation is built using OMNeT++, a discrete-event network simulator.

## System Architecture

The system consists of two independent nodes that exchange messages every 10 milliseconds (100 Hz):

Plant Node — Implements the system dynamics using a state-space model that evolves based on control inputs received from the controller node.

Controller Node — Contains two components:
- A PI (Proportional-Integral) controller that generates control commands to regulate the plant
- A Beard fault detection filter that monitors system behavior and identifies anomalies

## How It Works

The two nodes operate in a feedback loop: the plant node sends its current state to the controller, which processes this information through both the controller and fault detector, then sends back a control command. This cycle repeats at a 100 Hz update rate, simulating a real-time networked control system with integrated fault detection capabilities.
