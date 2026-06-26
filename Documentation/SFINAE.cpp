// SFINAE stands for "Substitution Failure Is Not An Error." It's a powerful C++ template metaprogramming technique that allows you to enable or disable function templates based on certain conditions, without causing compilation errors.
// This is particularly useful for creating more flexible and type-safe code, as it allows you to write function templates that only participate in overload resolution if certain criteria are met (e.g., if a type is integral, if it has a specific member function, etc.).
// The key idea is that when the compiler tries to instantiate a template with a specific type, if the substitution of that type into the template results in an invalid type or expression, the compiler doesn't treat it as an error. Instead, 
//it simply removes that template from the set of candidates for overload resolution and continues looking for other viable options. This allows you to write multiple versions of a function template that can handle different types or conditions without causing compilation failures.

// This is a core advanced concept. When the compiler tries to find a matching 
// function template, if a specific template doesn't "fit," the compiler doesn't 
// crash—it just quietly ignores that version and looks for another.

#include <iostream>
#include <type_traits>

// Version A: For classes that ARE integral (integers)
template <typename T>
typename std::enable_if<std::is_integral<T>::value, void>::type
checkType(T){
    std::cout << "This is an integer type.\n";
}

// Version B:For classes that ARE NOT integral (like strings or custom classes)
template <typename T>
typename std::enable_if<!std::is_integral<T>::value, void>::type
checkType(T){
    std::cout << "This is NOT an integer type.\n";
}

int main(){
    checkType(10);      // Calls Version A
    checkType("Hello"); // Calls Version B
    return 0;
}