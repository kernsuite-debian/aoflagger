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
  pytest ../../../test/integration/tBase.py
"""

MS = f"{config.RESOURCE_DIR}/base/3C196_spw5_sub1-WSRT.MS"


def test():
    # Prepare and validate input.
    # Note the uploaded data set should already be in this state, but this
    # guards against other tests modifying the data set.
    check_call([config.taql, "update", MS, "set", "FLAG=False"])
    check_call([config.aoquality, "collect", MS])
    result = check_output([config.aoquality, "summarizerfi", MS])
    assert result == b"4703949275\t4703953055\t146.85\t149.34\t0\n"
    check_call([config.taql, "delete", "from", f"{MS}/HISTORY"])
    assert_taql(f"select from {MS}/HISTORY")

    # Run Aoflagger and validate output.
    check_call([config.aoflagger, MS])
    check_call([config.aoquality, "collect", MS])
    result = check_output([config.aoquality, "summarizerfi", MS])
    # In the prepare step the code expects exactly 0% of the data flagged.
    # For this test we want to validate an approximate value:
    # - avoids difference between platform's floating point types to cause
    #   false positives,
    # - avoids small changes in the code to directly fail the test.
    assert re.match(
        b"4703949275\t4703953055\t146.85\t149.34\t19.5[0-9]+\n", result
    )
    # Did we write one history record?
    assert_taql(f"select from {MS}/HISTORY", 1)

    # Only test gui components if they have been built
    if config.enable_gui == "ON":
        # Test rfigui save baseline functionality (parameters: filename, a1, a2, band, sequence)
        # The output isn't checked -- this is just to see if the command succeeds.
        check_call(
            [
                config.rfigui,
                MS,
                "-save-baseline",
                "baseline-test.pdf",
                "0",
                "1",
                "0",
                "0",
            ]
        )

        # Test if the statistic plots (pdf files) can be saved.
        check_call([config.aoqplot, MS, "-save", "test", "StandardDeviation"])
