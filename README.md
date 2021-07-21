Project developed for the Parallel Programming Models course of the University of Pisa

To employ the framework, include one of the 4 headers available: frame_threads_1D.hpp, frame_threads_2D.hpp, frameFF_DM2D.hpp or frameFF_mw_1D.hpp
To use one of the last two headers you will have to download the Fastflow library and include it at compile time.
There are 4 more header files, which are used in the framework headers. These are ints_1D_t.hpp, ints_2D_t.hpp, cells_1D_t.hpp, cells_2D_t.hpp. 
You can freely include either a cell header or an int header in each of the 4 initial headers. To use the framework in your application, after including it, you will have to subclass the main class "Game" and provide it with an suitable implementation of the virtual method rule. Now you should be able to instantiate objects of the subclass and call its method run(steps) to perform the rule steps time, and print() to visualize the current state of the automaton.