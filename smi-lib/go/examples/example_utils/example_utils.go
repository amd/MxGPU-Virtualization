// Copyright Advanced Micro Devices, Inc.
//
// SPDX-License-Identifier: MIT

// Package example_utils contains input and error-reporting helpers shared by
// the focused AMD SMI Go examples.
package example_utils

import (
	"bufio"
	"context"
	"errors"
	"fmt"
	"io"
	"os"
	"strconv"
	"strings"

	"smi-lib/go/amdsmi"
)

// InputOptions describes the accepted values for PromptForInput.
type InputOptions struct {
	DefaultValue uint64
	LowerBound   uint64
	UpperBound   *uint64
	ValidValues  []uint64
	ErrorMessage string
	RequireInput bool
}

type readResult struct {
	line string
	err  error
}

// Prompter reads interactive example input and writes prompts and validation
// messages.
type Prompter struct {
	results <-chan readResult
	writer  io.Writer
}

// NewPrompter creates an interactive example prompter.
func NewPrompter(reader io.Reader, writer io.Writer) *Prompter {
	results := make(chan readResult, 1)
	bufferedReader := bufio.NewReader(reader)
	go func() {
		defer close(results)
		for {
			line, err := bufferedReader.ReadString('\n')
			results <- readResult{line: line, err: err}
			if err != nil {
				return
			}
		}
	}()

	return &Prompter{
		results: results,
		writer:  writer,
	}
}

// PromptForInput reads an unsigned integer that satisfies the supplied
// constraints. Unless RequireInput is set, an empty response selects the
// default value.
func (p *Prompter) PromptForInput(
	ctx context.Context,
	prompt string,
	options InputOptions,
) (uint64, error) {
	if !isValidInput(options.DefaultValue, options) {
		return 0, fmt.Errorf("default input value %d is not valid", options.DefaultValue)
	}

	errorMessage := options.ErrorMessage
	if errorMessage == "" {
		errorMessage = inputErrorMessage(options)
	}

	promptText := prompt
	if !options.RequireInput {
		promptText = fmt.Sprintf("%s[default: %d] ", prompt, options.DefaultValue)
	}

	for {
		input, err := p.readLine(ctx, promptText)
		if err != nil {
			return 0, err
		}
		if input == "" {
			if !options.RequireInput {
				return options.DefaultValue, nil
			}
			fmt.Fprintln(p.writer, errorMessage)
			continue
		}

		value, err := strconv.ParseUint(input, 10, 64)
		if err == nil && isValidInput(value, options) {
			return value, nil
		}

		fmt.Fprintln(p.writer, errorMessage)
	}
}

// PromptConfirmation asks for an explicit yes or no response.
func (p *Prompter) PromptConfirmation(ctx context.Context, prompt string) (bool, error) {
	for {
		input, err := p.readLine(ctx, prompt+" (YES/no) ")
		if err != nil {
			return false, err
		}

		switch strings.ToLower(input) {
		case "yes":
			return true, nil
		case "no":
			return false, nil
		default:
			fmt.Fprintln(p.writer, "  Enter yes or no")
		}
	}
}

// PromptForLine reads one trimmed line of free-form input through the shared
// stdin reader so callers do not open a second bufio.Reader on the same stream.
func (p *Prompter) PromptForLine(ctx context.Context, prompt string) (string, error) {
	return p.readLine(ctx, prompt)
}

// HandleExampleError reports the result of an example run. A nil error returns
// without printing. context.Canceled prints a keyboard-interrupt message and
// returns. Any other error is written to stderr and the process exits with
// status 1. AMD SMI status errors are printed as the description from
// StatusCodeToString when that decode succeeds.
func HandleExampleError(err error) {
	if err == nil {
		return
	}
	if errors.Is(err, context.Canceled) {
		fmt.Println("\n  Keyboard interrupt detected. Exiting...")
		return
	}

	var statusErr *amdsmi.StatusError
	if errors.As(err, &statusErr) {
		description, decodeErr := amdsmi.StatusCodeToString(statusErr.Code)
		if decodeErr != nil {
			fmt.Fprintln(os.Stderr, err)
		} else {
			fmt.Fprintln(os.Stderr, description)
		}
	} else {
		fmt.Fprintln(os.Stderr, err)
	}
	os.Exit(1)
}

// PrintOptionalAPIError reports common capability and environment errors
// without forcing the caller to stop the rest of an example. It returns true
// only when err is an expected optional-capability error.
func PrintOptionalAPIError(writer io.Writer, apiName string, err error) bool {
	var statusErr *amdsmi.StatusError
	if !errors.As(err, &statusErr) {
		return false
	}

	switch statusErr.Code {
	case amdsmi.AMDSMI_STATUS_NOT_SUPPORTED,
		amdsmi.AMDSMI_STATUS_DRIVER_NOT_LOADED,
		amdsmi.AMDSMI_STATUS_NO_PERM,
		amdsmi.AMDSMI_STATUS_NOT_FOUND,
		amdsmi.AMDSMI_STATUS_NO_DATA:
		printOptionalAPIErrorMessage(writer, apiName, err)
		return true
	default:
		return false
	}
}

// GetOptionalAPI calls api and returns its result. On error it prints a
// user-friendly message and returns the zero value of T with false, matching
// Python get_optional_api() returning None.
func GetOptionalAPI[T any](
	writer io.Writer,
	apiName string,
	api func() (T, error),
) (T, error) {
	result, err := api()
	if err == nil {
		return result, nil
	}

	printOptionalAPIErrorMessage(writer, apiName, err)
	var zero T
	return zero, err
}

// SetOptionalAPI calls api and returns AMDSMI_STATUS_SUCCESS on success. On
// error it prints a user-friendly message and returns the status code,
// matching Python set_optional_api().
func SetOptionalAPI(
	writer io.Writer,
	apiName string,
	api func() error,
) amdsmi.Status {
	err := api()
	if err == nil {
		return amdsmi.AMDSMI_STATUS_SUCCESS
	}

	printOptionalAPIErrorMessage(writer, apiName, err)
	var statusErr *amdsmi.StatusError
	if errors.As(err, &statusErr) {
		return statusErr.Code
	}
	return amdsmi.AMDSMI_STATUS_UNKNOWN_ERROR
}

func printOptionalAPIErrorMessage(writer io.Writer, apiName string, err error) {
	var statusErr *amdsmi.StatusError
	if !errors.As(err, &statusErr) {
		fmt.Fprintf(writer, "  %s failed: %v\n", apiName, err)
		return
	}

	switch statusErr.Code {
	case amdsmi.AMDSMI_STATUS_NOT_SUPPORTED:
		fmt.Fprintf(writer, "  %s is not supported on this system\n", apiName)
	case amdsmi.AMDSMI_STATUS_DRIVER_NOT_LOADED:
		fmt.Fprintf(writer, "  %s requires the appropriate device driver\n", apiName)
	case amdsmi.AMDSMI_STATUS_NO_PERM:
		fmt.Fprintf(
			writer,
			"  %s requires permission to access the AMD SMI driver device\n",
			apiName,
		)
		fmt.Fprintln(writer, "  From the smi-lib repository root, run:\n")
		fmt.Fprintln(writer, "  sudo go run go/examples/<example>/amdsmi_<example>.go")
	case amdsmi.AMDSMI_STATUS_NOT_FOUND:
		fmt.Fprintf(writer, "  %s found no matching object\n", apiName)
	case amdsmi.AMDSMI_STATUS_NO_DATA:
		fmt.Fprintf(writer, "  %s returned no data\n", apiName)
	case amdsmi.AMDSMI_STATUS_INVAL:
		fmt.Fprintf(writer, "  %s received invalid parameters\n", apiName)
	default:
		fmt.Fprintf(writer, "  %s failed: %v\n", apiName, err)
	}
}

func (p *Prompter) readLine(ctx context.Context, prompt string) (string, error) {
	if err := ctx.Err(); err != nil {
		return "", err
	}
	fmt.Fprint(p.writer, prompt)

	select {
	case <-ctx.Done():
		return "", ctx.Err()
	case result, ok := <-p.results:
		if !ok {
			return "", io.EOF
		}
		if result.err != nil &&
			!(errors.Is(result.err, io.EOF) && result.line != "") {
			return "", fmt.Errorf("read input: %w", result.err)
		}
		return strings.TrimSpace(result.line), nil
	}
}

func isValidInput(value uint64, options InputOptions) bool {
	if value < options.LowerBound {
		return false
	}
	if options.UpperBound != nil && value > *options.UpperBound {
		return false
	}
	if len(options.ValidValues) == 0 {
		return true
	}
	for _, validValue := range options.ValidValues {
		if value == validValue {
			return true
		}
	}
	return false
}

func inputErrorMessage(options InputOptions) string {
	if len(options.ValidValues) > 0 {
		values := make([]string, len(options.ValidValues))
		for i, value := range options.ValidValues {
			values[i] = strconv.FormatUint(value, 10)
		}
		return "  Input must be one of: " + strings.Join(values, ", ")
	}
	if options.UpperBound != nil {
		return fmt.Sprintf(
			"  Input must be between %d and %d",
			options.LowerBound,
			*options.UpperBound,
		)
	}
	return fmt.Sprintf("  Input must be at least %d", options.LowerBound)
}
