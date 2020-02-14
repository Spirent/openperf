package openperf_test

import (
	"testing"

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"
)

func TestOpenperf(t *testing.T) {
	RegisterFailHandler(Fail)
	RunSpecs(t, "Openperf Suite")
}
