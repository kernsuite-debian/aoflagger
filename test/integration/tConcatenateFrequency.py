import pytest
import re
import sys
from subprocess import check_call, check_output

# Append current directory to system path in order to import testconfig
sys.path.append(".")
import testconfig as config
from utils import assert_taql, untar_ms

"""
Basic integration test.

Script should be invoked like
  cd $BUILD_DIR/test/integration
  pytest ../../../test/integration/tConcatenateFrequency.py
"""

MSs = [
    f"{config.RESOURCE_DIR}/concatenate_frequency/L228163_SB150_uv.dppp.MS",
    f"{config.RESOURCE_DIR}/concatenate_frequency/L228163_SB151_uv.dppp.MS",
]


@pytest.mark.parametrize(
    "read_mode",
    ["", "-direct-read", "-indirect-read", "-memory-read", "-auto-read-mode"],
)
def test(read_mode):
    # Prepare and validate input.
    # Note the uploaded data set should already be in this state, but this
    # guards against other tests modifying the data set.
    for MS in MSs:
        check_call([config.taql, "update", MS, "set", "FLAG=False"])
        check_call([config.aoquality, "collect", MS])
        result = check_output([config.aoquality, "summarizerfi", MS])
        # Only validate the flagged percentage
        assert re.match(b".*\t0\n", result)
        check_call([config.taql, "delete", "from", f"{MS}/HISTORY"])
        assert_taql(f"select from {MS}/HISTORY")

    # Run Aoflagger and validate output.
    # The MSs are passed in reversed order to check that aoflagger sorts
    # them along frequency before concatenating
    if read_mode != "":
        check_call(
            [config.aoflagger, "-concatenate-frequency", read_mode] + MSs[::-1]
        )
    else:
        check_call([config.aoflagger, "-concatenate-frequency"] + MSs[::-1])

    for MS in MSs:
        check_call([config.aoquality, "collect", MS])
        # Did we write one history record?
        assert_taql(f"select from {MS}/HISTORY", 1)

    # In the prepare step the code expects exactly 0% of the data flagged.
    # For this test we want to validate an approximate value:
    # - avoids difference between platform's floating point types to cause
    #   false positives,
    # - avoids small changes in the code to directly fail the test.
    # Don't validate the other fields in this test.
    result = check_output([config.aoquality, "summarizerfi", MSs[0]])
    assert re.match(b".*\t0.638[0-9]+\n", result)
    result = check_output([config.aoquality, "summarizerfi", MSs[1]])
    assert re.match(b".*\t1.061[0-9]+\n", result)


# The output has been validated by running AOFlagger twice on the same data:
# 1. intervals 0 - 75
# 2. intervals 75 - 150
# The rest of the test is the same as the original test so comments have been removed.
@pytest.mark.parametrize(
    "chunk_size",
    ["75", "100", "149"],
)
def test_two_chunks(chunk_size):
    for MS in MSs:
        check_call([config.taql, "update", MS, "set", "FLAG=False"])
        check_call([config.aoquality, "collect", MS])
        result = check_output([config.aoquality, "summarizerfi", MS])

    check_call(
        [config.aoflagger, "-concatenate-frequency", "-chunk-size", chunk_size]
        + MSs
    )

    for MS in MSs:
        check_call([config.aoquality, "collect", MS])

    result = check_output([config.aoquality, "summarizerfi", MSs[0]])
    assert re.match(b".*\t0.569[0-9]+\n", result)
    result = check_output([config.aoquality, "summarizerfi", MSs[1]])
    assert re.match(b".*\t0.984[0-9]+\n", result)


# Like the previous, but using 3 chunks and limit the interval used.
# The three intervals used are:
# 1. interval 30 - 53
# 2. interval 53 - 76
# 3. interval 76 - 99
@pytest.mark.parametrize(
    "chunk_size",
    ["23", "34"],
)
def test_three_chunks_limited_interval(chunk_size):
    for MS in MSs:
        check_call([config.taql, "update", MS, "set", "FLAG=False"])
        check_call([config.aoquality, "collect", MS])
        result = check_output([config.aoquality, "summarizerfi", MS])

    check_call(
        [
            config.aoflagger,
            "-concatenate-frequency",
            "-interval",
            "30",
            "99",
            "-chunk-size",
            chunk_size,
        ]
        + MSs
    )

    for MS in MSs:
        check_call([config.aoquality, "collect", MS])

    result = check_output([config.aoquality, "summarizerfi", MSs[0]])
    assert re.match(b".*\t0.175[0-9]+\n", result)
    result = check_output([config.aoquality, "summarizerfi", MSs[1]])
    assert re.match(b".*\t0.384[0-9]+\n", result)
