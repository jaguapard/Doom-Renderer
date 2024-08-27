#pragma once

template <class TypeInput, class TypeOutput>
class ShaderBase
{
public:
	TypeOutput virtual run(TypeInput& input) = 0;
};