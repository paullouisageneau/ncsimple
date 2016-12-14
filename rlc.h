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

#ifndef NC_RLC_H
#define NC_RLC_H

#include <iostream>
#include <map>
#include <list>
#include <cstddef>

namespace nc
{

// Modify if necessary
typedef unsigned char  uint8_t;
typedef unsigned long  uint64_t;

// Optimized XOR
void memxor(char *a, const char *b, size_t size);

// Pseudo-random linear coding implementation
class Rlc
{
public:
	static void Init(void);
	static void Cleanup(void);

	class Combination
	{
	public:
		Combination(void);
		Combination(const Combination &combination);
		Combination(unsigned offset, const char *data = NULL, size_t size = 0);
		~Combination(void);
		
		void addComponent(unsigned offset, uint8_t coeff);
		void addComponent(unsigned offset, uint8_t coeff, const char *data, size_t size);
		void setData(const char *data, size_t size);
		void setCodedData(const char *data, size_t size);
		
		unsigned firstComponent(void) const;
		unsigned lastComponent(void) const;
		unsigned componentsCount(void) const;
		uint8_t coeff(unsigned offset) const;

		bool isCoded(void) const;
		bool isNull(void) const;
		bool isLast(void) const;
		
		const char *data(void) const;
		size_t size(void) const;
		size_t codedSize(void) const;

		void clear(void);
		
		Combination &operator=(const Combination &combination);
		Combination operator+(const Combination &combination) const;
		Combination operator*(uint8_t coeff) const;
		Combination operator/(uint8_t coeff) const;
		Combination &operator+=(const Combination &combination);
		Combination &operator*=(uint8_t coeff);
		Combination &operator/=(uint8_t coeff);
		
	private:
		void resize(size_t size, bool zerofill = false);

		std::map<unsigned, uint8_t> mComponents;
		char *mData = NULL;
		size_t mSize;
	};
	
	Rlc(uint64_t seed = 0);
	~Rlc(void);
	
	// Source
	int add(const char *data, size_t size);		// Add component from data	
	bool generate(Combination &output);		// Generate combination
	void clear(void);				// Clear system

	// Sink
	bool solve(Combination incoming);		// Add combination and try to solve, return true if innovative
	int get(std::list<const Combination*> &decoded) const;		// Get all combinations	
	int getDecoded(std::list<const Combination*> &decoded) const;	// Get decoded combinations	

	unsigned seenCount(void) const;			// Return seen combinations count (degree)
	unsigned decodedCount(void) const;		// Return decoded combinations count
	unsigned componentsCount(void) const;		// Return number of components in system
	unsigned size(void) const { return seenCount(); }

	size_t dump(std::ostream &os) const;		// Dump data from decoded combinations
	void print(std::ostream &os) const;		// Print current system
	
private:
	// Pseudo-random generator
	class Generator
	{
	public:
		Generator(uint64_t seed);
		~Generator(void);
		uint8_t next(void);
	
	private:
		uint64_t mSeed;
	};

	// GF(2^8) operations
	static uint8_t gAdd(uint8_t a, uint8_t b);
	static uint8_t gMul(uint8_t a, uint8_t b); 
	static uint8_t gInv(uint8_t a);
	
	// GF(2^8) operations tables
	static uint8_t *MulTable;
	static uint8_t *InvTable;

	std::map<unsigned, Combination> mCombinations;	// combinations sorted by pivot component
	unsigned mDecodedCount;
	unsigned mComponentsCount;
	Generator mGen;
};

std::ostream &operator<< (std::ostream &s, const Rlc::Combination &c);

}

#endif
