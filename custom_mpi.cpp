#include "custom_mpi.h"

void custom_all_gather(boost::mpi::communicator& comm,
                       std::vector<std::shared_ptr<Timetable>>& input_values,
                       std::vector<std::shared_ptr<Timetable>>& destination_values) {
    int rank = comm.rank();
    int size = comm.size();

    std::vector<std::shared_ptr<Timetable>> transfer_buffer = std::vector<std::shared_ptr<Timetable>>();
    for (int i = 0; i < size; i++) {
        transfer_buffer.clear();
        if (rank == i) {
            transfer_buffer.insert(transfer_buffer.end(), input_values.begin(), input_values.end());
        }

        boost::mpi::broadcast(comm, transfer_buffer, i);
        destination_values.insert(destination_values.end(), transfer_buffer.begin(), transfer_buffer.end());
    }
}
