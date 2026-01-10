BEGIN {
    FS = "\t";
}

(NR == 1) {
    printf("<thead>\n")
    printf("<tr>")
    for (i = 1; i <= NF; i++) {
        printf("<th scope=\"column\" style=\"text-align:center\">%s</th>\n", $i);
    }
    printf("</tr>\n");
    printf("</thead>\n");
    printf("<tbody>\n");
}

(NR > 1) {
    printf("<tr>");
    printf("<th scope=\"row\">%s</th>", $1);
    for (i = 2; i <= NF; i++) {
        printf("<td>%s</td>", $i);
    }
    printf("</tr>\n");
}

END {
    printf("</tbody>\n");
}