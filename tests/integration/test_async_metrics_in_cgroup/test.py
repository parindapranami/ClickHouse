import pytest
import subprocess
import time

from helpers.cluster import ClickHouseCluster

cluster = ClickHouseCluster(__file__)
node = cluster.add_instance("node")


@pytest.fixture(scope="module")
def start_cluster():
    try:
        cluster.start()
        yield cluster
    finally:
        cluster.shutdown()


def test_check_client_logs_level(start_cluster):
    # check that our metrics sources actually exist
    assert (
        subprocess.Popen("test -f /sys/fs/cgroup/cpu.stat".split(" ")).wait() == 0
        or subprocess.Popen(
            "test -f /sys/fs/cgroup/cpuacct/cpuacct.stat".split(" ")
        ).wait()
        == 0
    )

    # first let's spawn some cpu-intensive process outside of the container and check that it doesn't accounted by ClickHouse server
    proc = subprocess.Popen(
        "openssl speed -multi 8".split(" "),
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )

    time.sleep(5)

    metric = node.query(
        """
      SYSTEM FLUSH LOGS;

      SELECT max(value)
        FROM (
          SELECT toStartOfInterval(event_time, toIntervalSecond(1)) AS t, avg(value) AS value
              FROM system.asynchronous_metric_log
          WHERE event_time >= now() - 60 AND metric = 'OSUserTime'
          GROUP BY t
        )
    """
    ).strip("\n")

    assert float(metric) <= 2

    proc.kill()

    # then let's test that we will account cpu time spent by the server itself
    node.query(
        "SELECT cityHash64(*) FROM system.numbers_mt FORMAT Null SETTINGS max_execution_time=5, max_threads=8",
        ignore_error=True,
    )

    metric = node.query(
        """
      SYSTEM FLUSH LOGS;

      SELECT max(value)
        FROM (
          SELECT toStartOfInterval(event_time, toIntervalSecond(1)) AS t, avg(value) AS value
              FROM system.asynchronous_metric_log
          WHERE event_time >= now() - 60 AND metric = 'OSUserTime'
          GROUP BY t
        )
    """
    ).strip("\n")

    assert 4 <= float(metric) <= 12
