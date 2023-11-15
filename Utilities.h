#ifndef hUtilities
#define hUtilities

template <typename T>
inline static T lerp(T v0, T v1, float t)
{// v0 initial value, v1 final value, t input to interpolate
	//	t is a float from 0 to 1
	//		t can be replaced with a function of value t
	//		that returns a value between 0 to 1, the shape of t
	//		changes the interpolated value.
	return (1 - t) * v0 + t * v1;
}

#endif