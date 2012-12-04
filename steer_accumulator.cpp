/*
 * steer_accumulator.cpp
 * 
 * Copyright 2012 Florian Jung <florian.a.jung@web.de>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 3
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */


#include "steer_accumulator.h"

using namespace std;

void SteerAccumulator::add_steerer(SteerIface* steerer, double weight)
{
	steerers.push_back(entry(steerer,weight));
}

void SteerAccumulator::process_image(const Mat& img)
{
	for (list<entry>::iterator it = steerers.begin(); it!=steerers.end(); ++it)
		it->st->process_image(img);
}

double SteerAccumulator::get_steer_data()
{
	double sum=0;
	for (list<entry>::iterator it = steerers.begin(); it!=steerers.end(); ++it)
		sum+=it->st->get_steer_data() * it->st->get_confidence() * it->weight;
	
	double confidence = get_confidence();
	
	return (confidence==0) ? 0.0 : (sum/confidence);
}

double SteerAccumulator::get_confidence()
{
	double sum=0;
	for (list<entry>::iterator it = steerers.begin(); it!=steerers.end(); ++it)
		sum+=it->st->get_confidence() * it->weight;
	return sum;
}
