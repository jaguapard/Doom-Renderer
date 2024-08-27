#pragma once

template <class TypeInput, class TypeOutput>
class ShaderBase
{
public:
	virtual TypeOutput run(TypeInput& input) = 0;
};