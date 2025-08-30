const gulp = require('gulp');
const htmlmin = require('gulp-htmlmin');
const inlineSource = require('gulp-inline-source');
const del = require('del');

const paths = {
    src: 'src/index.html',
    dest: 'build', // Output to the parent directory (main)
    destFile: 'build/index.html'
};

// Clean the destination file
const clean = () => del([paths.destFile], { force: true });

// Build task: inline CSS/JS and minify everything into a single HTML file
const build = () => {
    return gulp.src(paths.src)
        .pipe(inlineSource({
            rootpath: './src/', // Base path for assets is the current directory
            compress: true
        }))
        .pipe(htmlmin({
            collapseWhitespace: true,
            removeComments: true,
            minifyCSS: true,
            minifyJS: true
        }))
        .pipe(gulp.dest(paths.dest))
        .pipe(gulp.dest("../main")); 
};

const watch = () => {
    gulp.watch(paths.src, build);
};

exports.clean = clean;
exports.build = build;
exports.watch = watch;

exports.default = gulp.series(clean, build);