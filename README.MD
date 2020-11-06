# Simple Server/Client

Example of simple server/client that can work with both UPD and TCP protocols

## Requirements
  + Linux
  + Compiler with C++11 support
  + CMake >= 3.0.0

## Description

### Technologies used

Neither server nor client uses additional threads and both based on event listening technologies.

Server uses 'epoll' method to work with multiple UPD and TCP sessions at same time

Client works with one server by chosen protocol and uses 'select' method to listen events.

Both application has been provided with interative menus so you don't get lost.

### Main functionality

Server handles text messages from client. If message contains numbers then server returns sum of that numbers, otherwise it returns the same message.
