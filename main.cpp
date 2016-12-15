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

#include <string>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cmath>

int main(int argc, char **argv)
{	
	nc::Rlc::Init();	// Global RLC initialization

	// ========== RLC test ==========
	const char *p1 = "Ceci est le premier paquet.\n";
	const char *p2 = "Ceci est le deuxième paquet.\n";
	const char *p3 = "Et voilà le troisième paquet.\n";

	unsigned long seed = (unsigned long)(time(NULL));
	nc::Rlc source(seed);
	source.add(p1, std::strlen(p1));
	source.add(p2, std::strlen(p2));
	source.add(p3, std::strlen(p3));

	nc::Rlc sink;
	for(int i=0; i<3; ++i)	// Transmit combinations from source to sink
	{
		nc::Rlc::Combination c;
		source.generate(c);

		// Combination c should be sent through the network

		// To send, coded data can be accessed with c.data() and c.codedSize()
		// and coefficients with c.coeff(i) with i between 0 and c.lastComponent()

		// At reception, combination can be rebuilt with c.setCodedData(data, size)
		// and multiple calls to c.addComponent(i, coefficient)

		std::cout << "Received: " << c << std::endl;
		sink.solve(c);

		std::cout << "System:" << std::endl;
		sink.print(std::cout);
		std::cout << std::endl;
	}

	std::cout << "Listing combinations: " << std::endl;
	std::list<const nc::Rlc::Combination*> lst;
	//sink.getDecoded(lst);	// Get decoded
	sink.get(lst);		// Get all
	while(!lst.empty())
	{
		std::cout << *lst.front() << ": " << std::string(lst.front()->data(), lst.front()->size());
		lst.pop_front();
	}

	std::cout << "Total decoded: " << sink.decodedCount() << std::endl << std::endl;

	std::cout << "Dumping packets: " << std::endl;
	sink.dump(std::cout);

	nc::Rlc::Cleanup();	// Global RLC cleanup
	return 0;
}


