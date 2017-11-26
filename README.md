# opo-c

OpO C Client

[OpO](http://opo.technology" is a fast triple store that is ideally suited for
storing JSON documents. It has an HTTP API as well as more direct API. The OpO
Client is a C client that provides access to that API.

The OpO Client provides basic connect, query, and close functions. The queries
are [TQL](pages/doc/tql/index.html) JSON. They can be constructed with the
[OjC](https://github.com/ohler55/ojc) library or more efficiently with the
opoBuilder. All queries are made asynchronously but can be made synchronous
with a simple wrapper function that makes use of the opo_client_process()
function.

While query responses are asynchronous, processing of the responses is
controlled by the caller with the opo_client_process() function. In the unit
test a simple thread is created that continually processes responses but the
call can be made one at a time for more control over response handling.

