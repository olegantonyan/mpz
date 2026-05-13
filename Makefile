PORT ?= 8867

serve:
	python3 -m http.server $(PORT)

.PHONY: serve
