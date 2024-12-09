# docker/frontend/Dockerfile

# ============================
# Stage 1: Build the frontend
# ============================
FROM node:18-alpine AS builder

# Set working directory
WORKDIR /app

# Set custom npm registry
RUN npm config set registry https://registry.npmmirror.com

# Install pnpm globally
RUN npm install -g pnpm

# Set custom PNPM mirror
ENV PNPM_REGISTRY=https://registry.npmmirror.com

# Copy package files and install dependencies
COPY ui/frontend/package.json ui/frontend/pnpm-lock.yaml ./

RUN pnpm config set registry $PNPM_REGISTRY

# Copy the rest of the frontend code
COPY ui/frontend/ ./

RUN pnpm install --frozen-lockfile

# Build the frontend for production
RUN pnpm run build

# ============================
# Stage 2: Serve with Nginx
# ============================
FROM nginx:stable-alpine

# Remove the default Nginx static assets
RUN rm -rf /usr/share/nginx/html/*

# Copy built frontend from builder stage
COPY --from=builder /app/dist /usr/share/nginx/html

# Copy custom Nginx configuration
COPY docker/frontend/nginx.conf /etc/nginx/conf.d/default.conf

# Expose port 80
EXPOSE 80

# Start Nginx
CMD ["nginx", "-g", "daemon off;"]
