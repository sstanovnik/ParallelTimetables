## Parallel Timetables
This is my bachelors thesis project. I researched and implemented an evolutionary algorithm for generating university timetables. 
Parallelization was achieved with MPI. 

A script to set up a fresh Ubuntu 15.04 (desktop or server) machine for compiling and running the program is included. Also included is an HTML timetable viewer for analyzing results. 

### Features
 - XML Schema Definitions for input data. 
 - A robust and easily extensible system for implementing new constraints. 
 - An efficient parallelization with minimized memory usage. 
 - A machine set-up script. 
 - A launch script tat handles copying, builds and launches on multiple machines at once. 
 - Editing algorithm parameters without recompiling the program. 
 - An input generator for generating inputs with specific properties. 
 - Result viewer (HTML application). 

#### Dependencies
 - Boost (with mandatory compiled libraries)
    - Boost.Serialization
    - Boost.MPI

#### Included libraries
 - [TinyXML2](https://github.com/leethomason/tinyxml2)
 - [JSON for Modern C++](https://github.com/nlohmann/json)
 - [Bootstrap](http://getbootstrap.com/)
 - [jQuery](https://jquery.com/)
 - [AngularJS](https://angularjs.org/)


### License
The source code is licensed under the MIT license. The thesis is licenced under CC BY-SA 4.0. See included license files for details.
