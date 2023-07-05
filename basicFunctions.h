#ifndef hBasicFunctions
#define hBasicFunctions


template<auto mVar>
auto getMember(auto& obj) {
    return obj.*mVar;
}

/// <summary>
/// Prints the input to the console.
/// </summary>
/// <param name="a">= function input</param>
void print(auto a) {
    std::cout << a << std::endl;
}

#endif