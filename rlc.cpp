/****************************************************************************
 *   Copyright (C) 2013-2016 by Paul-Louis Ageneau                          *
 *   paul-louis (at) ageneau (dot) org                                      *
 *                                                                          *
 *   This file is part of NC-Simple.                                        *
 *                                                                          *
 *   NC-Simple is free software: you can redistribute it and/or modify      *
 *   it under the terms of the GNU General Public License as published by   *
 *   the Free Software Foundation, either version 3 of the License, or      *
 *   (at your option) any later version.                                    *
 *                                                                          *
 *   NC-Simple is distributed in the hope that it will be useful,           *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *   GNU General Public License for more details.                           *
 *                                                                          *
 *   You should have received a copy of the GNU General Public License      *
 *   along with NC-Simple. If not, see <http://www.gnu.org/licenses/>.      *
 ****************************************************************************/

#include "rlc.h"

#include <stdexcept>
#include <cassert>

namespace nc
{

void memxor(char *a, const char *b, size_t size)
{
	unsigned long *la = reinterpret_cast<unsigned long*>(a);
	const unsigned long *lb = reinterpret_cast<const unsigned long*>(b);
	const size_t n = size / sizeof(unsigned long);
	for(size_t i = 0; i < n; ++i)
		la[i]^= lb[i];
	for(size_t i = n*sizeof(unsigned long); i < size; ++i)
		a[i]^= b[i];
}

uint8_t *Rlc::MulTable = NULL;
uint8_t *Rlc::InvTable = NULL;

void Rlc::Init(void)
{
	if(!MulTable) 
	{
		MulTable = new uint8_t[256*256];
		
		MulTable[0] = 0;
		for(uint8_t i = 1; i != 0; ++i) 
		{
			MulTable[unsigned(i)] = 0;
			MulTable[unsigned(i)*256] = 0;
			
			for(uint8_t j = 1; j != 0; ++j)
			{
				uint8_t a = i;
				uint8_t b = j;
				uint8_t p = 0;
				uint8_t k;
				uint8_t carry;
				for(k = 0; k < 8; ++k)
				{
					if (b & 1) p^= a;
					carry = (a & 0x80);
					a<<= 1;
					if (carry) a^= 0x1b; // 0x1b is x^8 modulo x^8 + x^4 + x^3 + x + 1
					b>>= 1;
				}
				
				MulTable[unsigned(i)*256+unsigned(j)] = p;
			}
		}
	}
	
	if(!InvTable)
	{
		InvTable = new uint8_t[256];
		
		InvTable[0] = 0;
		for(uint8_t i = 1; i != 0; ++i)
		{
			for(uint8_t j = i; j != 0; ++j)
			{
				if(Rlc::gMul(i,j) == 1)	// then Rlc::gMul(j,i) == 1
				{
					InvTable[i] = j;
					InvTable[j] = i;
				}
			}
		}
	}
}

void Rlc::Cleanup(void)
{
	delete[] MulTable;
	delete[] InvTable;
	MulTable = NULL;
	InvTable = NULL;
}

uint8_t Rlc::gAdd(uint8_t a, uint8_t b)
{
	return a ^ b;
}

uint8_t Rlc::gMul(uint8_t a, uint8_t b) 
{
	return MulTable[unsigned(a)*256+unsigned(b)];
}

uint8_t Rlc::gInv(uint8_t a) 
{
	return InvTable[a];
}

Rlc::Generator::Generator(uint64_t seed) :
	mSeed(seed)
{

}

Rlc::Generator::~Generator(void)
{

}

uint8_t Rlc::Generator::next(void)
{
	if(!mSeed) return 1;
	
	uint8_t value;
	do {
		// Knuth's 64-bit linear congruential generator
		mSeed = uint64_t(mSeed*6364136223846793005L + 1442695040888963407L);
		value = uint8_t(mSeed >> 56);
	}
	while(!mSeed || !value);	// zero is not a valid output

	return value;
}

Rlc::Combination::Combination(void) :
	mData(NULL),
	mSize(0)
{
	
}

Rlc::Combination::Combination(const Combination &combination) :
	mData(NULL),
	mSize(0)
{
	*this = combination;
}

Rlc::Combination::Combination(unsigned offset, const char *data, size_t size) :
	mData(NULL),
	mSize(0)
{
	addComponent(offset, 1, data, size);
}

Rlc::Combination::~Combination(void)
{
	clear();
}

void Rlc::Combination::addComponent(unsigned offset, uint8_t coeff)
{
	std::map<unsigned, uint8_t>::iterator it = mComponents.find(offset);
	if(it != mComponents.end())
	{
		it->second = Rlc::gAdd(it->second, coeff);
		if(it->second == 0)
			mComponents.erase(it);
	}
	else {
		if(coeff != 0)
			mComponents[offset] = coeff;
	}
}

void Rlc::Combination::addComponent(unsigned offset, uint8_t coeff, const char *data, size_t size)
{
	addComponent(offset, coeff);
	
	if(!mSize && coeff == 1)
	{
		setData(data, size);
		return;
	}
	
	// Assure mData is the longest vector
	if(mSize < size+1) // +1 for padding
		resize(size+1, true);	// zerofill
	
	if(coeff == 1)
	{
		// Add values
		//for(unsigned i = 0; i < size; ++i)
		//	mData[i] = Rlc::gAdd(mData[i], data[i]);
	  
	  	// Faster
		memxor(mData, data, size);
		mData[size]^= 0x80; // 1-byte padding
	}
	else {
		if(coeff == 0) return;
		
		// Add values
		//for(unsigned i = 0; i < size; ++i)
		//	mData[i] = Rlc::gAdd(mData[i], Rlc::gMul(data[i], coeff));
		
		// Faster
		for(unsigned i = 0; i < size; ++i)
			mData[i]^= Rlc::gMul(data[i], coeff);
		
		mData[size]^= Rlc::gMul(0x80, coeff); // 1-byte padding
	}
}

void Rlc::Combination::setData(const char *data, size_t size)
{
	resize(size+1, false);	// +1 for padding
	std::copy(data, data + size, mData);
	mData[size] = 0x80;	// 1-byte ISO/IEC 7816-4 padding with fin flag
}

void Rlc::Combination::setCodedData(const char *data, size_t size)
{
	resize(size, false);
	std::copy(data, data + size, mData);
}

unsigned Rlc::Combination::firstComponent(void) const
{
	if(!mComponents.empty()) return mComponents.begin()->first;
	else return 0;
}

unsigned Rlc::Combination::lastComponent(void) const
{
	if(!mComponents.empty()) return (--mComponents.end())->first;
	else return 0;
}

unsigned Rlc::Combination::componentsCount(void) const
{
	if(!mComponents.empty()) return (lastComponent() - firstComponent()) + 1;
	else return 0;
}

uint8_t Rlc::Combination::coeff(unsigned offset) const
{
	std::map<unsigned, uint8_t>::const_iterator it = mComponents.find(offset);
	if(it == mComponents.end()) return 0; 
	return it->second;
}

bool Rlc::Combination::isCoded(void) const
{
	return (mComponents.size() != 1 || mComponents.begin()->second != 1);
}

bool Rlc::Combination::isNull(void) const
{
	return (mComponents.size() == 0);
}

const char *Rlc::Combination::data(void) const
{
	return mData;
}

size_t Rlc::Combination::size(void) const
{
	if(!mSize || isCoded())
		return mSize;
	
	size_t size = mSize - 1;
	while(size && !mData[size])
		--size;
	
	if(mData[size] != char(0x80) && mData[size] != char(0x81))
		throw std::runtime_error("Data corruption in RLC: invalid padding");
	
	return size;
}

size_t Rlc::Combination::codedSize(void) const
{
	return mSize;
}


void Rlc::Combination::clear(void)
{
	mComponents.clear();
	delete[] mData;
	mData = NULL;
	mSize = 0;
}

Rlc::Combination &Rlc::Combination::operator=(const Combination &combination)
{
	mComponents = combination.mComponents;
	resize(combination.mSize);
	std::copy(combination.mData, combination.mData + combination.mSize, mData);
	return *this;
}

Rlc::Combination Rlc::Combination::operator+(const Combination &combination) const
{
	Rlc::Combination result(*this);
	result+= combination;
	return result;
}

Rlc::Combination Rlc::Combination::operator*(uint8_t coeff) const
{
	Rlc::Combination result(*this);
	result*= coeff;
	return result;
}

Rlc::Combination Rlc::Combination::operator/(uint8_t coeff) const
{
	Rlc::Combination result(*this);
	result/= coeff;	
	return result;
}
	
Rlc::Combination &Rlc::Combination::operator+=(const Combination &combination)
{
	// Assure mData is long enough
	if(mSize < combination.mSize)
		resize(combination.mSize, true);	// zerofill
	
	// Add data
	//for(size_t i = 0; i < combination.mSize; ++i)
	//	mData[i] = Rlc::gAdd(mData[i], combination.mData[i]);

	// Faster
	memxor(mData, combination.mData, combination.mSize);
	
	// Add components
	for(	std::map<unsigned, uint8_t>::const_iterator jt = combination.mComponents.begin();
		jt != combination.mComponents.end();
		++jt)
	{
		addComponent(jt->first, jt->second);
	}
	
	return *this;
}

Rlc::Combination &Rlc::Combination::operator*=(uint8_t coeff)
{
	if(coeff != 1)
	{
		if(coeff != 0)
		{
			// Multiply data
			for(size_t i = 0; i < mSize; ++i)
				mData[i] = Rlc::gMul(mData[i], coeff);

			for(std::map<unsigned, uint8_t>::iterator it = mComponents.begin(); it != mComponents.end(); ++it)
				it->second = Rlc::gMul(it->second, coeff);
		}
		else {
			std::fill(mData, mData + mSize, 0);
			mComponents.clear();
		}
	}
	
	return *this;
}

Rlc::Combination &Rlc::Combination::operator/=(uint8_t coeff)
{
	assert(coeff != 0);

	(*this)*= Rlc::gInv(coeff);
	return *this;
}

void Rlc::Combination::resize(size_t size, bool zerofill)
{
	if(mSize != size)
	{
		char *newData = new char[size];
		std::copy(mData, mData + std::min(mSize, size), newData);
		if(zerofill && size > mSize)
			std::fill(newData + mSize, newData + size, 0);
		
		delete[] mData;
		mData = newData;
		mSize = size;
	}
}

Rlc::Rlc(uint64_t seed) :
	mDecodedCount(0),
	mComponentsCount(0),
	mGen(seed)
{

}

Rlc::~Rlc(void)
{
	
}

int Rlc::add(const char *data, size_t size)
{
	mCombinations[mComponentsCount].addComponent(mComponentsCount, 1, data, size);
	return mComponentsCount++;
}

bool Rlc::generate(Combination &output)
{	
	output.clear();
	
	if(mCombinations.empty())
		return false;
	
	for(std::map<unsigned, Combination>::const_iterator it = mCombinations.begin();
		it != mCombinations.end();
		++it)
	{
		uint8_t coeff = mGen.next();
		output+= it->second*coeff;
	}
	
	return true;
}

void Rlc::clear(void)
{
	mCombinations.clear();
	mDecodedCount = 0;
	mComponentsCount = 0;
}

bool Rlc::solve(Combination incoming)
{
	if(incoming.isNull())
		return false;
	
	mComponentsCount = std::max(mComponentsCount, incoming.lastComponent()+1);
	
	// ==== Gauss-Jordan elimination ====
	
	std::map<unsigned, Combination>::iterator it, jt;
	std::map<unsigned, Combination>::reverse_iterator rit;
	
	// Eliminate coordinates, so the system is triangular
	for(unsigned i = incoming.firstComponent(); i <= incoming.lastComponent(); ++i)
	{
		uint8_t c = incoming.coeff(i);
		if(c != 0)
		{
			jt = mCombinations.find(i);
			if(jt == mCombinations.end()) break;
			incoming+= jt->second*c;
		}
	}
	
	if(incoming.isNull())
		return false;	// non-innovative combination
	
	// Insert incoming combination
	incoming/= incoming.coeff(incoming.firstComponent());
	mCombinations[incoming.firstComponent()] = incoming;
	
	// Attempt to substitute to solve
	rit = mCombinations.rbegin();
	while(rit != mCombinations.rend())
	{
		unsigned first = std::max(rit->second.firstComponent(), rit->first);
		for(unsigned i = rit->second.lastComponent(); i > first; --i)
		{
			jt = mCombinations.find(i);
			if(jt != mCombinations.end())
			{
				if(jt->second.isCoded()) break;
				rit->second+= jt->second*rit->second.coeff(i);
			}
		}

		if(rit->second.lastComponent() != rit->first)
			break;
		
		++rit;
	}
	
	// Remove null components and count decoded
	mDecodedCount = 0;
	it = mCombinations.begin();
	while(it != mCombinations.end())
	{
		if(it->second.isNull())
		{
			// Null vector, useless equation
			mCombinations.erase(it++);
		}
		else {
			if(!it->second.isCoded())
				++mDecodedCount;
			
			++it;
		}
	}
	
	return true;	// incoming was innovative
}

int Rlc::get(std::list<const Combination*> &combinations) const
{
	combinations.clear();
	for(std::map<unsigned, Combination>::const_iterator it = mCombinations.begin();
		it != mCombinations.end();
		++it)
	{
		combinations.push_back(&it->second);
	}

	return combinations.size();
}

int Rlc::getDecoded(std::list<const Combination*> &decoded) const
{
	decoded.clear();
	for(std::map<unsigned, Combination>::const_iterator it = mCombinations.begin();
		it != mCombinations.end();
		++it)
	{
		if(!it->second.isCoded())
			decoded.push_back(&it->second);
	}

	return decoded.size();
}

size_t Rlc::dump(std::ostream &os) const
{
	size_t total = 0;
	for(std::map<unsigned, Combination>::const_iterator it = mCombinations.begin();
		it != mCombinations.end();
		++it)
	{
		if(!it->second.isCoded())
		{
			os.write(it->second.data(), it->second.size());
			total+= it->second.size();
		}
	}

	return total;
}


void Rlc::print(std::ostream &os) const
{
	for(std::map<unsigned, Combination>::const_iterator it = mCombinations.begin();
		it != mCombinations.end();
		++it)
	{
		os << it->second << std::endl;
	}
}

unsigned Rlc::seenCount(void) const
{
	return mCombinations.size();
}

unsigned Rlc::decodedCount(void) const
{
	return mDecodedCount;
}

unsigned Rlc::componentsCount(void) const
{
	return mComponentsCount;
}

std::ostream &operator<<(std::ostream &s, const Rlc::Combination &c)
{
	s << "combination (";
	if(!c.isNull())
		for(unsigned i = 0; i <= c.lastComponent(); ++i)
			s << (i ? ", " : "") << unsigned(c.coeff(i));
	s << ")";
	return s;
}

}
