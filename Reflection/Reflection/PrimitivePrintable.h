#pragma once

template <typename T>
class PrimitivePrintable : public IPrintable
{
public:
	explicit PrimitivePrintable(T& value)
		: mValue(value) {
	}

	void print() const override
	{
		if constexpr (std::is_same_v<T, bool>)
		{
			std::cout << (mValue ? "true" : "false");
		}
		else if constexpr (std::is_same_v<T, std::string>)
		{
			std::cout << mValue;
		}
		else
		{
			std::cout << mValue;
		}
	}

private:
	T& mValue;
};