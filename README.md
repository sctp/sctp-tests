# sctp-tests

This is sctp-tests, a test suite which includes a bunch of test
cases that are used mainly for regression testing on SCTP patch
development. It's the minimum set of tests that a patch is expected to
pass before posting to netdev@.

It currently supports running on physical hosts and also on net
namespaces. Ideally, both should be tested, as the dynamics of them are
different and may trigger some different behavior on the stack. For
example, there is no SCTP GRO, so on physical hosts, the stack will
never receive a packet larger than MTU, yet that is possible when using
veth pairs as the actual segmentation is skept.

Test cases are written in bash scripts to make it easy. You can
reference to testcase/misc/demo for a working example on how it's
written.

Test cases are also collected in test plans, which are tailored for
specific situations, mainly to save time during initial testing and/or
skip tests that are not compatible with some environment.

**BE WARNED:** The suite here has to be ran as root as it has the need to
run tons of administrative commands, and it will also automatically
install all software it requires for functioning, like netperf.

If you have any doubts or issues with it, please feel free to reach us
at linux-sctp@vger.kernel.org.

Alright, time to get our hands dirty!

### How To Run:

```
# ./sctp-tests run
or with any opts:
# ./sctp-tests -f st.cfg -e testcase/misc/demo/test_on_netns.sh \
               -l DBG -o xx -L /tmp/st.log -O /tmp/o.log run
```

### How To Debug:

```
# ./sctp-tests shell
# cd $casedir
  (ln -s xx.sh test.sh if it doesn't exist)
# st_setup
# st_test ; st_test ; st_test ; ...
# st_clean
# exit
```

### Log Files:

- sctp-tests.log (st log)
- /tmp/output.log (case output)
- $casedir/xxx_$(st_o)_yyy.log (local log)

### How To Use Phost Topo:

- add host and nic info in topos/ph.cfg
- run "./libst/bin/psshkey" to set sshkey and validate hosts
- start your test cases as usual

### Todo List:

- add more Topos like vlan, bonding, bridge ... and their combinations.
- add the reproducers of old sctp issues into Regression.
- test cases for the new sctp features into Conformance.
