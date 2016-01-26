/*
 * NewOrderMessage.hpp
 *
 *  Created on: 23-Jan-2016
 *      Author: ankithbti
 */

#ifndef NEWORDERMESSAGE_HPP_
#define NEWORDERMESSAGE_HPP_

#include <boost/fusion/adapted/struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

struct NewOrderMessage
{
	std::string _symbol;
	std::string _secDesc;
	double _price;
	int _quantity;
	char _side;
};

BOOST_FUSION_ADAPT_STRUCT(
		NewOrderMessage,
		(std::string, _symbol)
		(std::string, _secDesc)
		(double, _price)
		(int, _quantity)
		(char, _side)
		);

#endif /* NEWORDERMESSAGE_HPP_ */
