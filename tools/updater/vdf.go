package main

import (
	"fmt"
	"strings"
	"unicode"
)

// vdfNode is a node in a parsed VDF tree.
// Either Value is set (leaf) or Children is set (section).
type vdfNode struct {
	Key      string
	Value    string
	Children []*vdfNode
}

// vdfParse parses a VDF text into a tree of nodes.
func vdfParse(input string) ([]*vdfNode, error) {
	p := &vdfParser{input: []rune(input)}
	nodes, err := p.parseBlock()
	if err != nil {
		return nil, err
	}
	return nodes, nil
}

// vdfSerialize converts a VDF tree back to text.
func vdfSerialize(nodes []*vdfNode, indent int) string {
	var sb strings.Builder
	prefix := strings.Repeat("\t", indent)
	for _, n := range nodes {
		if n.Children != nil {
			fmt.Fprintf(&sb, "%s%s\n%s{\n", prefix, vdfQuote(n.Key), prefix)
			sb.WriteString(vdfSerialize(n.Children, indent+1))
			fmt.Fprintf(&sb, "%s}\n", prefix)
		} else {
			fmt.Fprintf(&sb, "%s%s\t\t%s\n", prefix, vdfQuote(n.Key), vdfQuote(n.Value))
		}
	}
	return sb.String()
}

func vdfQuote(s string) string {
	return `"` + strings.ReplaceAll(s, `"`, `\"`) + `"`
}

// findOrCreate walks a path of keys, creating sections as needed.
func vdfFindOrCreate(nodes *[]*vdfNode, keys ...string) *[]*vdfNode {
	if len(keys) == 0 {
		return nodes
	}
	key := keys[0]
	for _, n := range *nodes {
		if strings.EqualFold(n.Key, key) && n.Children != nil {
			return vdfFindOrCreate(&n.Children, keys[1:]...)
		}
	}
	// Not found — create it
	newNode := &vdfNode{Key: key, Children: []*vdfNode{}}
	*nodes = append(*nodes, newNode)
	return vdfFindOrCreate(&newNode.Children, keys[1:]...)
}

// vdfSet sets a leaf value at the given path, creating sections as needed.
func vdfSet(nodes *[]*vdfNode, value string, keys ...string) {
	if len(keys) == 0 {
		return
	}
	if len(keys) == 1 {
		// Set leaf at current level
		for _, n := range *nodes {
			if strings.EqualFold(n.Key, keys[0]) && n.Children == nil {
				n.Value = value
				return
			}
		}
		*nodes = append(*nodes, &vdfNode{Key: keys[0], Value: value})
		return
	}
	parent := vdfFindOrCreate(nodes, keys[:len(keys)-1]...)
	vdfSet(parent, value, keys[len(keys)-1])
}

// ── Parser ────────────────────────────────────────────────────────────────────

type vdfParser struct {
	input []rune
	pos   int
}

func (p *vdfParser) parseBlock() ([]*vdfNode, error) {
	var nodes []*vdfNode
	for {
		p.skipWhitespaceAndComments()
		if p.pos >= len(p.input) {
			break
		}
		if p.input[p.pos] == '}' {
			break
		}

		key, err := p.parseString()
		if err != nil {
			return nil, err
		}

		p.skipWhitespaceAndComments()
		if p.pos >= len(p.input) {
			return nil, fmt.Errorf("unexpected end of file after key %q", key)
		}

		if p.input[p.pos] == '{' {
			// Section
			p.pos++ // consume '{'
			children, err := p.parseBlock()
			if err != nil {
				return nil, err
			}
			p.skipWhitespaceAndComments()
			if p.pos < len(p.input) && p.input[p.pos] == '}' {
				p.pos++ // consume '}'
			}
			nodes = append(nodes, &vdfNode{Key: key, Children: children})
		} else {
			// Key-value pair
			val, err := p.parseString()
			if err != nil {
				return nil, err
			}
			nodes = append(nodes, &vdfNode{Key: key, Value: val})
		}
	}
	return nodes, nil
}

func (p *vdfParser) parseString() (string, error) {
	p.skipWhitespaceAndComments()
	if p.pos >= len(p.input) {
		return "", fmt.Errorf("unexpected end of file")
	}

	if p.input[p.pos] == '"' {
		return p.parseQuoted()
	}
	return p.parseUnquoted()
}

func (p *vdfParser) parseQuoted() (string, error) {
	p.pos++ // consume opening quote
	var sb strings.Builder
	for p.pos < len(p.input) {
		c := p.input[p.pos]
		if c == '"' {
			p.pos++
			return sb.String(), nil
		}
		if c == '\\' && p.pos+1 < len(p.input) {
			p.pos++
			switch p.input[p.pos] {
			case '"':
				sb.WriteRune('"')
			case '\\':
				sb.WriteRune('\\')
			case 'n':
				sb.WriteRune('\n')
			case 't':
				sb.WriteRune('\t')
			default:
				sb.WriteRune('\\')
				sb.WriteRune(p.input[p.pos])
			}
		} else {
			sb.WriteRune(c)
		}
		p.pos++
	}
	return "", fmt.Errorf("unterminated string")
}

func (p *vdfParser) parseUnquoted() (string, error) {
	start := p.pos
	for p.pos < len(p.input) && !unicode.IsSpace(p.input[p.pos]) &&
		p.input[p.pos] != '{' && p.input[p.pos] != '}' {
		p.pos++
	}
	if p.pos == start {
		return "", fmt.Errorf("expected string at position %d", p.pos)
	}
	return string(p.input[start:p.pos]), nil
}

func (p *vdfParser) skipWhitespaceAndComments() {
	for p.pos < len(p.input) {
		c := p.input[p.pos]
		if unicode.IsSpace(c) {
			p.pos++
			continue
		}
		// C++ style comments
		if c == '/' && p.pos+1 < len(p.input) && p.input[p.pos+1] == '/' {
			for p.pos < len(p.input) && p.input[p.pos] != '\n' {
				p.pos++
			}
			continue
		}
		break
	}
}
