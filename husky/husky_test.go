package main

import (
	"fmt"
	"net/http"
	"net/http/httptest"
	"strconv"
	"strings"
	"testing"
)

func TestCheckMergebotHealth(t *testing.T) {
	healthy := checkMergebotHealth(MERGEBOT_HOST, MERGEBOT_LISTEN_PORT)
	if healthy {
		t.Errorf("expected false, got %v", healthy)
	}
}

func TestCheckMergebotHealthWithMockServer(t *testing.T) {
	// Start a mock server at 127.0.0.1:18080/api/sa/health and return a 200 OK response
	// Call CheckMergebotHealth() and assert that it returns true
	ts := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
		w.Write([]byte("OK"))
	}))
	defer ts.Close()

	host, port, _ := parseHostAndPort(ts.URL)

	healthy := checkMergebotHealth(host, port)
	if !healthy {
		t.Errorf("expected true, got %v", healthy)
	}
}

func parseHostAndPort(url string) (string, int, error) {
	// Parse the URL and extract the host and port for testing
	var host string
	var port int
	_, err := fmt.Sscanf(url, "http://%s", &host)
	if err != nil {
		return "", 0, err
	}
	segs := strings.Split(host, ":")
	if len(segs) != 2 {
		return "", 0, fmt.Errorf("invalid URL: %s", url)
	}
	host = segs[0]
	port, err = strconv.Atoi(segs[1])
	if err != nil {
		return "", 0, err
	}
	return host, port, nil
}
