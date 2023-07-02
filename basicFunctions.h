#ifndef hBasicFunctions
#define hBasicFunctions

struct testS1 {
    int a = 100;
};

struct testS2 {
    bool b = false;
};

testS1 tS1;
testS2 tS2;

/// <summary>
/// Set of functions created to test out C++ features
/// </summary>
namespace MCFS {
    /// <summary>
    /// Returns a template specified member variable of a struct/class object.
    /// </summary>
    /// <typeparam name="mVar"></typeparam>
    /// <param name="obj">= struct/class </param>
    /// <returns> obj.mVar = struct/class member variable </returns>
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
}

#endif