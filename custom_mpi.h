#ifndef INCLUDE_CUSTOM_MPI_H
#define INCLUDE_CUSTOM_MPI_H

#include "timetable.h"
#include <boost/mpi.hpp>
#include <vector>

/**
 * A custom implementation of the Boost MPI_Allgather function.
 * Makes everyone have all data, regardless of the input sizes - even different across processors.
 */
void custom_all_gather(boost::mpi::communicator& comm,
                       std::vector<std::shared_ptr<Timetable>>& input_values,
                       std::vector<std::shared_ptr<Timetable>>& destination_values);

#endif //INCLUDE_CUSTOM_MPI_H
