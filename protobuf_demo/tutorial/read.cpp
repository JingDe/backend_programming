#include<iostream>
#include<fstream>
#include<string>
#include"addressbook.pb.h"

using namespace std;

void ListPeople(const tutorial::AddressBook& address_book)
{
	for(int i=0; i<address_book.person_size(); i++)
	{
		const tutorial::Person& person = address_book.person(i);
		std::cout<<person.name() <<" "<<person.age();
	}
}

int main(int argc, char** argv)
{
	if(argc != 2)
	{
		return -1;
	}

	tutorial::AddressBook address_book;
	{
		std::fstream input(argv[1], ios::in | ios::binary);
		if(!address_book.ParseFromIstream(&input))
		{
			std::cerr<<"Failed to parse address book\n";
			return -1;
		}
		input.close();
	}

	ListPeople(address_book);

	return 0;	
}
