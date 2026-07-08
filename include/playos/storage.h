// PlayOS Platform API — Storage.
//
// Save and configuration paths. Required capability `storage.local` —
// every conformant target provides this.
// See playos-spec: RFC-0004, book/src/05-capability-model/03-required-capabilities.md.
#pragma once

namespace PlayOS {
namespace Storage {

// Absolute path where the application should store persistent save data.
// Typically per-application (e.g. <base>/saves/<app-id>/).
// The directory is guaranteed to exist after the first call.
const char* SavePath();

// Absolute path where the application should store configuration.
// Typically per-application (e.g. <base>/config/<app-id>/).
// The directory is guaranteed to exist after the first call.
const char* ConfigPath();

} // namespace Storage
} // namespace PlayOS
