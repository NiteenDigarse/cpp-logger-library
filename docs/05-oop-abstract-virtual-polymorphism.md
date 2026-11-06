05 --- OOP Foundations: Abstract Classes, Virtual Functions & Runtime Polymorphism
1. WHY Do We Need These Concepts in Logger?
The Logger must support multiple output destinations:
- Console output
- File output
- Potential future destinations, such as network output
The Logger should not need separate hard-coded logic for every
destination. Instead, different output classes should follow one common
contract.

C++ object-oriented programming provides the tools for this design:
- Inheritance
- Virtual functions
- Pure virtual functions
- Abstract classes
- Function overriding
- Runtime polymorphism
- Virtual destructors
These concepts allow the Logger to work through a common interface while
concrete output classes provide destination-specific behavior.
2. Inheritance
Inheritance allows a derived class to build on a base class.

Example:
class Vehicle {
public:
    void serviceInfo() {
        // Common behavior
    }
};

class Car : public Vehicle {
};
Car : public Vehicle means Car publicly inherits from Vehicle.

The derived class can use accessible inherited members of the base
class.

In the Logger project:
class ConsoleSink : public LogSink {
};
ConsoleSink publicly inherits from LogSink.

The public inheritance relationship expresses an "is-a" relationship: a
ConsoleSink is a kind of LogSink.
3. Virtual Functions
A virtual function is a base-class member function that supports dynamic
dispatch when called through a base-class pointer or reference.

Example:
class Vehicle {
public:
    virtual void start() {
        std::cout << "Vehicle starting\n";
    }
};
The virtual keyword enables runtime polymorphic behavior for this
function.

A derived class can override the function:
class Car : public Vehicle {
public:
    void start() override {
        std::cout << "Car starting\n";
    }
};
Why does virtual matter?
Consider:
Car car;
Vehicle* ptr = &car;

ptr->start();
Although ptr has the static type Vehicle*, it points to a Car
object.

Because start() is virtual, the overridden Car::start()
implementation is selected.

Expected output:
Car starting
The actual object's type determines the final overridden implementation.
4. Function Overriding
Definition
Function overriding occurs when a derived class provides an
implementation for a virtual member function inherited from a base
class, using a matching function signature.

Example:
class Vehicle {
public:
    virtual void start() {
        std::cout << "Vehicle starting\n";
    }
};

class Car : public Vehicle {
public:
    void start() override {
        std::cout << "Car starting\n";
    }
};
Both classes have a start() function. The derived class provides its
own implementation of the inherited virtual function.
The override keyword
override tells the compiler that the derived function is intended to
override a base-class virtual function.

Example:
void start() override;
If the function does not correctly match a virtual function in the base
class, the compiler reports an error.

Important distinction:
- The matching derived virtual function definition performs the
  override.
- override asks the compiler to verify that the intended override is
  valid.
Using override is strongly recommended for derived virtual functions.
5. Pure Virtual Functions
A pure virtual function is declared by placing = 0 in its declaration.

Example:
class Vehicle {
public:
    virtual void start() = 0;
};
This declares start() as a pure virtual function.

It expresses a requirement in the base-class interface: a concrete
derived class must provide an implementation before its object can be
created.

A pure virtual function does not necessarily mean that the base class
can contain no other implemented functions. Abstract classes can also
contain ordinary member functions and data members.
6. Abstract Classes
Definition
An abstract class is a class that cannot be instantiated directly. A
class is abstract if it has at least one pure virtual function for which
the class has no final overrider.

Example:
class Vehicle {
public:
    virtual void start() = 0;
};
This is an abstract class.

The following is invalid:
Vehicle vehicle; // Error: Vehicle is abstract.
Why?

The class has a pure virtual function and does not provide a final
implementation for it.
Abstract classes can still be used through pointers and references
Although a direct object cannot be created, a pointer or reference to
the abstract base class can refer to a concrete derived object.
class Car : public Vehicle {
public:
    void start() override {
        std::cout << "Car starting\n";
    }
};

Car car;
Vehicle& vehicle = car;

vehicle.start();
This is valid. The reference refers to a Car object.
7. Concrete Classes
Definition
A concrete class is a class that can be instantiated. For a derived
class to be concrete, it must have implementations for all inherited
pure virtual functions that remain unimplemented.

Example:
class Vehicle {
public:
    virtual void start() = 0;
};

class Car : public Vehicle {
public:
    void start() override {
        std::cout << "Car starting\n";
    }
};
Now:
Car car; // Valid: Car is concrete.
What if a derived class misses an implementation?
Consider:
class Vehicle {
public:
    virtual void start() = 0;
    virtual void stop() = 0;
};

class Car : public Vehicle {
public:
    void start() override {
        std::cout << "Car starting\n";
    }
};
Car has not implemented stop().

Therefore, Car is still abstract and this is invalid:
Car car; // Error: Car is still abstract.
Once Car implements stop() as well, it can become concrete.
8. Abstract Class vs Concrete Class
  Abstract class                      Concrete class
  Cannot be instantiated directly     Can be instantiated

  Has at least one pure virtual       All inherited pure virtual
function without a final overrider  functions have final overriders

  Defines an interface and possibly   Provides complete behavior needed
shared behavior                     for its objects
Example: LogSink                  Example: ConsoleSink, FileSink
An abstract class is not necessarily only a collection of function
declarations. It may also contain data members, constructors,
destructors and implemented functions.
9. Function Overloading vs Function Overriding
These concepts both involve functions with the same name, but they solve
different problems.
9.1 Function Overloading
Definition: Function overloading means declaring multiple functions
with the same name in the same scope, but with different parameter
lists.

Example:
class Calculator {
public:
    int add(int a, int b) {
        return a + b;
    }

    double add(double a, double b) {
        return a + b;
    }

    int add(int a, int b, int c) {
        return a + b + c;
    }
};
The compiler selects an overload based on the arguments and
overload-resolution rules.

Overloading is commonly described as compile-time polymorphism.
9.2 Function Overriding
Definition: Function overriding occurs when a derived class provides
a matching implementation for an inherited virtual function.

Example:
class Animal {
public:
    virtual void sound() = 0;
};

class Dog : public Animal {
public:
    void sound() override {
        std::cout << "Bark\n";
    }
};
Overriding is used for runtime polymorphism when calls are made through
base-class pointers or references.
Comparison
  Feature                 Overloading             Overriding
  Function name           Same                    Same

  Parameter list          Different               Matching signature

  Inheritance required    No                      Yes

  Virtual function        No                      Yes, for virtual
required                                        runtime dispatch

  Selection               Compile-time overload   Runtime dispatch
resolution              through base
pointer/reference
Main purpose            Same operation with     Different
different input forms   implementations behind
a common interface
A return type alone is not enough to overload a function. The parameter
list must differ in a way that forms a valid overload.
10. Runtime Polymorphism
Definition
Runtime polymorphism means that a virtual function call through a
base-class pointer or reference can execute the final overrider
associated with the actual object at runtime.

Example:
class Animal {
public:
    virtual void sound() = 0;
    virtual ~Animal() = default;
};

class Dog : public Animal {
public:
    void sound() override {
        std::cout << "Bark\n";
    }
};

class Cat : public Animal {
public:
    void sound() override {
        std::cout << "Meow\n";
    }
};

Dog dog;
Cat cat;

Animal* first = &dog;
Animal* second = &cat;

first->sound();  // Dog::sound()
second->sound(); // Cat::sound()
The pointer type is Animal* in both cases, but the actual objects are
different.

This allows one common interface to work with multiple concrete
implementations.
Mental model
             Animal*
             /     \
            /       \
          Dog       Cat
           |         |
        sound()   sound()
         Bark      Meow
The base pointer/reference provides the common interface. The actual
object determines which overridden virtual function runs.
11. Virtual Destructors
Why does a base class need a virtual destructor?
A derived object may be destroyed through a pointer to its base class.

Example:
class Base {
public:
    virtual ~Base() = default;
};

class Derived : public Base {
public:
    ~Derived() {
        // Derived cleanup
    }
};

Base* object = new Derived();
delete object;
The virtual destructor ensures that destruction starts with the
most-derived object and proceeds through the base subobjects.

Without a virtual destructor, deleting a derived object through a base
pointer is undefined behavior when the base destructor is non-virtual.

For polymorphic base classes intended to be deleted through base
pointers, a public virtual destructor is the normal design.
Understanding virtual ~LogSink() = default;
virtual ~LogSink() = default;
- ~LogSink() --- destructor of LogSink.
- virtual --- enables correct polymorphic destruction through a base
  pointer.
- = default --- asks the compiler to generate the default destructor
  definition.
This is already a definition. A separate .cpp destructor definition is
not required.

If only a declaration were written:
virtual ~LogSink();
then a matching definition would need to be provided in a source file.
12. Connecting These Concepts to Logger
The planned sink interface is:
class LogSink {
public:
    virtual ~LogSink() = default;

    virtual void write(const std::string& message) = 0;
    virtual void flush() = 0;
};
LogSink is abstract because it has two pure virtual functions.

Concrete sinks will implement both functions.

Conceptual design:
                 LogSink
             Abstract interface
              /           \
             /             \
      ConsoleSink         FileSink
       Concrete            Concrete
          |                   |
       std::cout          std::ofstream
ConsoleSink responsibilities
- Implement write() to send the message to console output.
- Implement flush() to flush the console output stream.
FileSink responsibilities
- Implement write() to send the message to the file stream.
- Implement flush() to flush the file stream.
The Logger can then coordinate output through the common LogSink
interface rather than hard-coding every destination.

This is the design goal; the concrete sink implementations still need to
be completed and tested.
13. Important Rules and Common Mistakes
1. A class with an unimplemented pure virtual function cannot be
   instantiated.
2. A derived class that leaves an inherited pure virtual function
   without a final overrider remains abstract.
3. virtual enables runtime dispatch for virtual calls through base
   pointers or references.
4. override verifies that the derived declaration actually overrides
   a base virtual function.
5. A function with the same name is not automatically an override; the
   signature and inheritance relationship matter.
6. Overloading does not require inheritance.
7. A polymorphic base class intended for deletion through base pointers
   should have a virtual destructor.
8. = 0 in a function declaration means pure virtual; it is not a
   normal function body.
9. = default on the destructor asks the compiler to generate its
   default definition; it is not the same as = 0.
10. Abstract classes can contain ordinary implemented functions and data
    members.
14. Quick Reference
  Term                                Short definition
  Inheritance                         A derived class acquires accessible
members and behavior from a base
class.

  Virtual function                    A base-class function supporting
dynamic dispatch.

  Pure virtual function               A virtual function declared with
= 0.

  Abstract class                      A class that cannot be instantiated
directly because it has a pure
virtual function without a final
overrider.

  Concrete class                      A class that can be instantiated.

  Overriding                          A derived class provides a matching
implementation for an inherited
virtual function.

  Overloading                         Multiple functions share a name but
have different parameter lists.

  override                          Compiler-checked indication that a
function is intended to override a
base virtual function.

  Runtime polymorphism                Virtual call dispatches to the
final overrider associated with the
actual object.

  Virtual destructor                  Supports correct destruction
through a base-class pointer.

  = default                         Requests a compiler-generated
default function definition.
= 0                               Declares a pure virtual function.
15. Progress
- [x] Inheritance discussed using Vehicle, Car and Bike examples.
- [x] Virtual functions discussed.
- [x] Pure virtual functions discussed.
- [x] Abstract classes discussed.
- [x] Concrete classes discussed.
- [x] Missing pure virtual implementation experiment discussed.
- [x] Function overriding discussed.
- [x] Function overloading compared with overriding.
- [x] Runtime polymorphism discussed.
- [x] Virtual destructor and = default discussed.
- [x] Concepts connected to the Logger's LogSink design.
- [ ] ConsoleSink implementation and tests.
- [ ] FileSink implementation and tests.
- [ ] Main Logger implementation and tests.
Next implementation topic: Complete LogSink, then implement
ConsoleSink with write() and flush().