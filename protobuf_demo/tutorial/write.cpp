#include<iostream>
#include<fstream>
#include<string>
#include"addressbook.pb.h"

using namespace std;

void PromptForAddress(tutorial::Person* person)
{
	std::cout<<"Enter person name: "<<endl;
	std::string name;
	std::cin>>name;
	person->set_name(name);

	int age;
	std::cin>>age;
	person->set_age(age);
}

int main(int argc, char** argv)
{
	if(argc != 2)
	{
		std::cerr<<"Usage: "<<argv[0]<<" ADDRESS_BOOK_FILE "<<std::endl;
		return -1;
	}

	tutorial::AddressBook address_book;
	{
		std::fstream input(argv[1], ios::in | ios::binary);
		if(!input)
		{
			std::cout<<argv[1]<<": File not found\n";
		}
		else if(!address_book.ParseFromIstream(&input))
		{
			std::cerr<<"Failed to parse book"<<std::endl;
			return -1;
		}
	}	

	PromptForAddress(address_book.add_person());
	{
		std::fstream output(argv[1], ios::out  |  ios::trunc);
		if(!address_book.SerializeToOstream(&output))
		{
			std::cerr<<"Failed to write address book\n";
			return -1;	
		}
	}

	return 0;
}
