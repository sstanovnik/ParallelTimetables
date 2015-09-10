#!/usr/bin/env bash
# launching across multiple machines
#  - permit running in firewall (this was a problem on mac, not ubuntu server)
#     - also a problem when startingon mac, but not running on it
#  - enable SSH access to host (will prompt for password if needed)
#     - if more than one host requires a password, everything will break, so it's a good idea
#       to put the master's public key into ~/.ssh/authorized_keys (via scp etc)
#  - the project shall reside in ~/mpi/ and the executables will be in the out/ subdirectory.

# comma-separated hosts, optional sh username for the host
REMOTE_HOSTS="user@host1,user@host2"

usage() {
    echo "Usage: $(basename $0) <copy|build|run> <target>"
}

if [[ "$#" -lt 1 ]]; then
    echo "Invalid number of parameters. "
    usage;
    exit 1
fi

command=$1
case ${command} in
    "copy")
        ifsbak=$IFS
        IFS=','
        for host in ${REMOTE_HOSTS}; do
            scp -r ../* ${host}:~/mpi/
        done
        IFS=${ifsbak}
    ;;
    "build")
        if [[ "$#" -ne 2 ]]; then
            echo "Invalid number of parameters. "
            usage;
            exit 2;
        fi

        target="$2";

        ifsbak=$IFS
        IFS=','
        for host in ${REMOTE_HOSTS}; do
            ssh -C ${host} "cd mpi && cmake . && make -j \"\$(nproc)\" $target"
        done
        IFS=${ifsbak}
    ;;
    "run")
        if [[ "$#" -ne 2 ]]; then
            echo "Invalid number of parameters. "
            usage;
            exit 2;
        fi

        target="$2";

        # --host specified the hosts that will run the software
        # --map-by ppr:1:core maps one process to each core
        mpirun --host "localhost,${REMOTE_HOSTS}" --map-by ppr:1:core ${target}
    ;;
    *)
        echo "Invalid command. "
        usage;
        exit 3;
    ;;
esac
