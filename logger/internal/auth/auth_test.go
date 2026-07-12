package auth

import (
	"net/http"
	"net/http/httptest"
	"path/filepath"
	"strings"
	"testing"

	"github.com/mxcd/bob-e-car/logger/internal/store"
)

func TestSessionSurvivesRestart(t *testing.T) {
	st, err := store.New(filepath.Join(t.TempDir(), "test.sqlite"))
	if err != nil {
		t.Fatal(err)
	}
	defer st.Close()

	a := New("bob", "secret", st)
	response := httptest.NewRecorder()
	a.HandleLogin(response, httptest.NewRequest("POST", "/api/login",
		strings.NewReader(`{"username":"bob","password":"secret"}`)))
	if response.Code != http.StatusNoContent {
		t.Fatalf("login failed: %d", response.Code)
	}
	cookie := response.Result().Cookies()[0]

	request := httptest.NewRequest("GET", "/api/status", nil)
	request.AddCookie(cookie)

	// "reboot": a fresh Auth over the same store must accept the cookie
	restarted := New("bob", "secret", st)
	if !restarted.authenticated(request) {
		t.Fatal("session did not survive restart")
	}

	// logout revokes it across restarts too
	logoutRequest := httptest.NewRequest("POST", "/api/logout", nil)
	logoutRequest.AddCookie(cookie)
	restarted.HandleLogout(httptest.NewRecorder(), logoutRequest)
	if New("bob", "secret", st).authenticated(request) {
		t.Fatal("logged-out session still valid after restart")
	}
}
