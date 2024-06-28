#pragma once

namespace EnumclassHelper
{
	//returns next value of enum e. Cycles back to the beginning if it goes above max. WARNING: e MUST have COUNT value at the end and be sequential!
	template <typename T>
	inline T next(const T& e)
	{
		return static_cast<T>((int64_t(e) + 1) % int64_t(T::COUNT));
	}

	//returns previous value of enum e. Cycles back to the end if it goes below 0. WARNING: e MUST have COUNT value at the end and be sequential!
	template <typename T> 
	inline T prev(const T& e)
	{
		return static_cast<T>((int64_t(e) - 1) % int64_t(T::COUNT));
	}
}