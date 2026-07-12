package auth

import (
	"crypto/rand"
	"crypto/subtle"
	"encoding/hex"
	"encoding/json"
	"log"
	"net/http"
	"strings"
	"sync"
	"time"

	"github.com/mxcd/bob-e-car/logger/internal/store"
)

const cookieName = "bob_session"
const sessionMaxAge = 30 * 24 * time.Hour

// Auth guards the dashboard with a single username/password from the
// environment. Session tokens are persisted in the sqlite store so the
// Pi's power cycle with the car never logs anyone out. Disabled entirely
// when no credentials are configured (dev).
type Auth struct {
	username string
	password string
	store    *store.Store
	mu       sync.Mutex
	sessions map[string]bool
}

func New(username, password string, st *store.Store) *Auth {
	a := &Auth{username: username, password: password, store: st, sessions: map[string]bool{}}
	tokens, err := st.ListSessions(time.Now().UnixMicro(), sessionMaxAge.Microseconds())
	if err != nil {
		log.Println("failed to load sessions:", err)
		return a
	}
	for _, token := range tokens {
		a.sessions[token] = true
	}
	return a
}

func (a *Auth) Enabled() bool { return a.username != "" && a.password != "" }

func (a *Auth) validCredentials(username, password string) bool {
	userOk := subtle.ConstantTimeCompare([]byte(username), []byte(a.username)) == 1
	passOk := subtle.ConstantTimeCompare([]byte(password), []byte(a.password)) == 1
	return userOk && passOk
}

func (a *Auth) authenticated(r *http.Request) bool {
	if cookie, err := r.Cookie(cookieName); err == nil {
		a.mu.Lock()
		ok := a.sessions[cookie.Value]
		a.mu.Unlock()
		if ok {
			return true
		}
	}
	// basic auth keeps curl / `just ota` working without a cookie dance
	if username, password, ok := r.BasicAuth(); ok {
		return a.validCredentials(username, password)
	}
	return false
}

// Middleware protects everything except the login page and login endpoint.
// Unauthenticated API/websocket requests get 401, pages redirect to login.
func (a *Auth) Middleware(next http.Handler) http.Handler {
	if !a.Enabled() {
		return next
	}
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		path := r.URL.Path
		// the login page and the static assets it needs
		if path == "/login.html" || path == "/theme.css" || path == "/icon.svg" ||
			(path == "/api/login" && r.Method == http.MethodPost) {
			next.ServeHTTP(w, r)
			return
		}
		if a.authenticated(r) {
			next.ServeHTTP(w, r)
			return
		}
		if strings.HasPrefix(path, "/api/") || path == "/ws" {
			http.Error(w, "unauthorized", http.StatusUnauthorized)
			return
		}
		http.Redirect(w, r, "/login.html", http.StatusFound)
	})
}

func (a *Auth) HandleLogin(w http.ResponseWriter, r *http.Request) {
	var body struct {
		Username string `json:"username"`
		Password string `json:"password"`
	}
	if err := json.NewDecoder(r.Body).Decode(&body); err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}
	if !a.Enabled() || !a.validCredentials(body.Username, body.Password) {
		time.Sleep(500 * time.Millisecond) // brute-force damper
		http.Error(w, "invalid credentials", http.StatusUnauthorized)
		return
	}
	token := make([]byte, 32)
	rand.Read(token)
	session := hex.EncodeToString(token)
	a.mu.Lock()
	a.sessions[session] = true
	a.mu.Unlock()
	if err := a.store.InsertSession(session, time.Now().UnixMicro()); err != nil {
		log.Println("failed to persist session:", err) // session still works until reboot
	}
	http.SetCookie(w, sessionCookie(session, int(sessionMaxAge.Seconds())))
	w.WriteHeader(http.StatusNoContent)
}

func (a *Auth) HandleLogout(w http.ResponseWriter, r *http.Request) {
	if cookie, err := r.Cookie(cookieName); err == nil {
		a.mu.Lock()
		delete(a.sessions, cookie.Value)
		a.mu.Unlock()
		if err := a.store.DeleteSession(cookie.Value); err != nil {
			log.Println("failed to delete session:", err)
		}
	}
	http.SetCookie(w, sessionCookie("", -1))
	w.WriteHeader(http.StatusNoContent)
}

func sessionCookie(value string, maxAge int) *http.Cookie {
	// SameSite=Lax doubles as CSRF protection for the POST/DELETE endpoints;
	// no Secure flag — the dashboard is served over plain http in the tailnet
	return &http.Cookie{
		Name: cookieName, Value: value, Path: "/",
		MaxAge: maxAge, HttpOnly: true, SameSite: http.SameSiteLaxMode,
	}
}
