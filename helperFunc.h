#ifndef hHelperFunctions
#define hHelperFunctions

template<auto mVar>
auto getMember(auto& obj) {
    return obj.*mVar;
}

template<typename T> struct member;

template<typename Class, typename mType>
struct member<mType Class::*>
{
    using type = mType;
};

template<auto mVar>
auto constexpr packMembers(auto&& srcVec) {
    std::vector<typename member<decltype(mVar)>::type> dstVec(srcVec.size());
    std::transform(srcVec.begin(), srcVec.end(), dstVec.begin(), [=](auto const& mSrc) { return (*mSrc).*mVar; });
    return dstVec;
}

template<auto mVar>
auto constexpr transformVector(auto& srcVec) {
    std::vector<typename member<decltype(mVar)>::type> dstVec(srcVec.size());
    std::transform(srcVec.begin(), srcVec.end(), dstVec.begin(), [=](auto const& mSrc) { return mSrc.*mVar; });
    return dstVec;
}

#endif