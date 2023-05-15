#pragma once

#include "CoreMinimal.h"
#include "Kismet/KismetMathLibrary.h"

template<class T>
class TRandomPermutationIterator
{
	TArray<T>& ArrayRef;
	TArray<int> Permutation;
	int CurrIndex = 0;

public:
	explicit TRandomPermutationIterator(TArray<T>& Array) : ArrayRef(Array)
	{
		const int N = ArrayRef.Num();
		Permutation = GetPermutationRange(0, N - 1);
	}

	// dereference
	T operator*() const { return ArrayRef[Permutation[CurrIndex]]; }

	// prefix increment
	TRandomPermutationIterator& operator++() { CurrIndex++; return *this; }

	// postfix increment
	TRandomPermutationIterator operator++(int) { TRandomPermutationIterator Tmp = *this; ++(*this); return Tmp; }

	// something like `iterator == _.end()`
	bool IsEnd() { return CurrIndex == ArrayRef.Num(); }

	static TArray<int> GetPermutationRange(const int Min, const int Max)
	{
		TArray<int> Result;
		TArray<int> AvailableElems;

		const int N = Max - Min + 1;
		AvailableElems.Reserve(N);

		for (int i = 0; i < N; i++)
		{
			AvailableElems.Add(Min + i);
		}

		// now AvailableElems == [Min, Min + 1, ..., Max]
		
		Result.Reserve(N);
		
		for (int i = 0; i < N; i++)
		{
			const int RandomIndex = UKismetMathLibrary::RandomIntegerInRange(0, AvailableElems.Num() - 1);
			Result.Add(AvailableElems[RandomIndex]);
			AvailableElems.RemoveAt(RandomIndex);
		}

		return Result;
	}
};
