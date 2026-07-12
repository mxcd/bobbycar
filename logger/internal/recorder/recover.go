package recorder

import (
	"log"
	"os"
	"path/filepath"

	"github.com/mxcd/bob-e-car/logger/internal/logfile"
	"github.com/mxcd/bob-e-car/logger/internal/store"
)

// Recover reconciles log metadata with the files on disk after an unclean
// shutdown (power cut is the normal shutdown in the car). Runs before the
// recorder starts, so no file is being written concurrently.
//
// - metadata row without a file → row deleted
// - unfinalized row, or size mismatch between row and file → file scanned,
//   truncated to the last complete record, metadata finalized from content
// - file without any complete record → file and row deleted
func Recover(st *store.Store, logDir string) {
	logs, err := st.ListLogs()
	if err != nil {
		log.Println("recovery: failed to list logs:", err)
		return
	}
	for _, meta := range logs {
		path := filepath.Join(logDir, meta.Filename)
		info, err := os.Stat(path)
		if os.IsNotExist(err) {
			log.Printf("recovery: %s is gone, removing metadata", meta.Filename)
			st.DeleteLog(meta.Id)
			continue
		}
		if err != nil {
			log.Println("recovery: stat failed:", err)
			continue
		}
		if meta.EndedAtUs != nil && info.Size() == meta.SizeBytes {
			continue // finalized and consistent
		}

		result, err := logfile.Scan(path)
		if err != nil {
			log.Println("recovery: scan failed:", err)
			continue
		}
		if result.RecordCount == 0 {
			log.Printf("recovery: %s has no complete records, deleting", meta.Filename)
			st.DeleteLog(meta.Id)
			os.Remove(path)
			continue
		}
		if result.ValidBytes < info.Size() {
			log.Printf("recovery: %s truncating %d partial tail bytes", meta.Filename, info.Size()-result.ValidBytes)
			if err := os.Truncate(path, result.ValidBytes); err != nil {
				log.Println("recovery: truncate failed:", err)
				continue
			}
		}
		if err := st.FinalizeLog(meta.Id, int64(result.LastUs), result.ValidBytes, result.RecordCount); err != nil {
			log.Println("recovery: finalize failed:", err)
			continue
		}
		log.Printf("recovery: %s closed (%d records, %d bytes)", meta.Filename, result.RecordCount, result.ValidBytes)
	}
}
