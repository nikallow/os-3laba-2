FROM texlive/texlive:latest

WORKDIR /work/report

CMD ["sh", "-c", "pdflatex -interaction=nonstopmode -halt-on-error report.tex && pdflatex -interaction=nonstopmode -halt-on-error report.tex"]
